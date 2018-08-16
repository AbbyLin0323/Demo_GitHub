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


LOCAL void HAL_SpiEnterQPI(void);
LOCAL void HAL_SpiEnterSPI(void);
LOCAL void HAL_SpiEnterDPI(void);
LOCAL void HAL_SpiEnter4B(void);
LOCAL void HAL_SpiExit4B(void);

LOCAL volatile SPI_REG_SET* const l_pSpiRegSet = (volatile SPI_REG_SET*)(REG_BASE_SPI);
LOCAL U32 l_ulCurrSpiMode;

LOCAL SPI_RWCMD_TABLE const l_aRCmdTable[] =
{
    RCMD_ENTRY_0,RCMD_ENTRY_1,RCMD_ENTRY_2,RCMD_ENTRY_3,
    RCMD_ENTRY_4,RCMD_ENTRY_5,RCMD_ENTRY_6,RCMD_ENTRY_7
};

LOCAL SPI_RWCMD_TABLE const l_aWCmdTable[] =
{
    WCMD_ENTRY_0,WCMD_ENTRY_1,WCMD_ENTRY_2,WCMD_ENTRY_3
};

LOCAL PFUNC const l_aSetSpiMode[] =
{
    HAL_SpiEnterSPI,
    HAL_SpiEnterQPI
};

LOCAL PFUNC const l_aSetAddrMode[] =
{
    HAL_SpiExit4B,
    HAL_SpiEnter4B
};


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
LOCAL void HAL_SpiSendCCmd(U8 ucCmdType,U8 ucRespByteNum,U8 ucCContByteNum)
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
FUNC: HAL_SpiEnterQPI
Input:  
Output: 
Description:
    1. Issue requeset to make SPI flash to enter QPI mode if necessary
    2. Set SNFC into QPI mode 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL void HAL_SpiEnterQPI(void)
{
    if (SPI_MODE == l_ulCurrSpiMode)
    {
        HAL_SpiSendCCmd(SPI_CMD_EQIO,0,0);
        l_pSpiRegSet->bsCIOMode = SPI_CMDIO_4BIT;
        l_pSpiRegSet->bsCSpiGen = SPI_DATAIO_4BIT;
        l_ulCurrSpiMode = QPI_MODE;
    }    
}
/******************************************************************************
FUNC: HAL_SpiEnterSPI
Input:  
Output: 
Description:
    1. Issue requeset to make SPI flash to enter SPI mode if necessary
    2. Set SNFC into SPI mode 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL void HAL_SpiEnterSPI(void)
{
    if (QPI_MODE == l_ulCurrSpiMode)
    {
        HAL_SpiSendCCmd(SPI_CMD_EXQIO,0,0);
        l_pSpiRegSet->bsCIOMode = SPI_CMDIO_1BIT;
        l_pSpiRegSet->bsCSpiGen = SPI_DATAIO_1BIT;
        l_ulCurrSpiMode = SPI_MODE;
    }     
}

/******************************************************************************
FUNC: HAL_SpiEnter4B
Input:  
Output: 
Description:
    1. Issue requeset to make SPI flash to enter 4B mode if necessary
    2. Set SNFC into 4B mode if necessary 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL void HAL_SpiEnter4B(void)
{
    if (l_pSpiRegSet->bsAddrMode != SPI_ADDR_4B)
    {
        HAL_SpiSendCCmd(SPI_CMD_EN4B,0,0);
        l_pSpiRegSet->bsAddrMode = SPI_ADDR_4B;
    }
}
/******************************************************************************
FUNC: HAL_SpiExit4B
Input:  
Output: 
Description:
    1. Issue requeset to make SPI flash to enter 3B mode if necessary
    2. Set SNFC into 3B mode 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL void HAL_SpiExit4B(void)
{
    if (l_pSpiRegSet->bsAddrMode != SPI_ADDR_3B)
    {
        HAL_SpiSendCCmd(SPI_CMD_EX4B,0,0);
        l_pSpiRegSet->bsAddrMode = SPI_ADDR_3B; 
    }
}
/******************************************************************************
FUNC: HAL_SpiReadId
Input:  
Output: 
Description:
    1. Read SPI Flash ID 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL U32 HAL_SpiReadId(void)
{
    l_pSpiRegSet->ulCRespDW[0] = 0;
    HAL_SpiSendCCmd(SPI_CMD_RDID,2,0);
    return l_pSpiRegSet->ulCRespDW[0];
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

LOCAL BOOL HAL_SpiGetWIP(void)
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

LOCAL BOOL HAL_SpiGetWEL(void)
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

LOCAL void HAL_SpiWriteEnable(void)
{
    HAL_SpiSendCCmd(SPI_CMD_WREN,0,0);    
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
GLOBAL U8 HAL_SpiReadStatus(U8 ulRSRCode)
{
    HAL_SpiSendCCmd(ulRSRCode,1,0);
    return (l_pSpiRegSet->ucCRespByte[0]);
}

/******************************************************************************
FUNC: HAL_SpiWriteStatus
Input:  U8 Write status register command code
        U32 ulData , input data
        U32 ulLenB , data length ,count in byte
Output:
Description:
    1. Write SPI Flash status register
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL void HAL_SpiWriteStatus(U8 ulWSRCode,U32 ulData,U32 ulLenB)
{
    HAL_SpiWriteEnable();
    while(FALSE == HAL_SpiGetWEL())
    {
        ;
    }
    l_pSpiRegSet->ulCContentDW[0] = ulData;
    HAL_SpiSendCCmd(ulWSRCode,0,ulLenB);
    while(TRUE == HAL_SpiGetWIP())
    {
        ;
    }
}
/******************************************************************************
FUNC: HAL_SpiSetRCmd
Input:  
Output: 
Description:
    1. Select read command
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL void HAL_SpiSetRCmd(U32 ulIndex)
{
    l_aSetSpiMode[l_aRCmdTable[ulIndex].bsSpiMode]();
    l_aSetAddrMode[l_aRCmdTable[ulIndex].bsAddrMode]();
    l_pSpiRegSet->bsRCmdCode = l_aRCmdTable[ulIndex].bsCmdCode;
    l_pSpiRegSet->bsRIOMode  = l_aRCmdTable[ulIndex].bsCmdIO;
    l_pSpiRegSet->bsRSpiGen  = l_aRCmdTable[ulIndex].bsDataIO;
    l_pSpiRegSet->bsDummy    = l_aRCmdTable[ulIndex].bsDummy;
}

/******************************************************************************
FUNC: HAL_SpiSetWCmd
Input:  
Output: 
Description:
    1. Select write command
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL void HAL_SpiSetWCmd(U32 ulIndex)
{
    l_aSetSpiMode[l_aWCmdTable[ulIndex].bsSpiMode]();
    l_aSetAddrMode[l_aWCmdTable[ulIndex].bsAddrMode]();
    l_pSpiRegSet->bsWCmdCode = l_aWCmdTable[ulIndex].bsCmdCode;
    l_pSpiRegSet->bsWIOMode  = l_aWCmdTable[ulIndex].bsCmdIO;
    l_pSpiRegSet->bsWSpiGen  = l_aWCmdTable[ulIndex].bsDataIO;
}


/******************************************************************************
FUNC: HAL_SetReadParameter
Input:  
Output: 
Description:
    1. Set Read Parameter
History:
    2014/12/03 Regina Wang  reconstruct

*******************************************************************************/

GLOBAL void HAL_SetReadParameter(U8 ulCode,U32 ulData,U32 ulLenB)
{
    l_pSpiRegSet->ulCContentDW[0] = ulData;   
    HAL_SpiSendCCmd(ulCode,0,ulLenB); 
}

/******************************************************************************
FUNC: HAL_ChipErase
Input:  
Output: 
Description:
    1. Erase Whole Chip
History:
    2014/12/03 Regina Wang  reconstruct

*******************************************************************************/

GLOBAL void HAL_ChipErase(void)
{
    HAL_SpiWriteEnable();
    while(FALSE == HAL_SpiGetWEL())
    {
        ;
    }
    
    HAL_SpiSendCCmd(SPI_CMD_CE,0,0);
    
    while(TRUE == HAL_SpiGetWIP())
    {
        ;
    }
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

LOCAL void HAL_SpiHWInit(void)
{
    COM_MemZero((U32 *) l_pSpiRegSet,sizeof(SPI_REG_SET)/sizeof(U32));  // reset first 7 dword
    l_pSpiRegSet->bsSnf       = TRUE;
    l_pSpiRegSet->bsRCmdEn    = TRUE;   
    l_ulCurrSpiMode = SPI_MODE;
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

LOCAL void HAL_SpiWaitRstFsh(void)
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
void HAL_SpiSoftReset(void)
{
    HAL_SpiSendCCmd(SPI_CMD_RSTEN,0,0);
    HAL_SpiSendCCmd(SPI_CMD_RST,0,0);  
    HAL_SpiWaitRstFsh();
    HAL_SpiHWInit();

    HAL_SpiSetWCmd(SPI_WCMD_INDEX_PP);
    HAL_SpiSetRCmd(SPI_RCMD_INDEX_READ);
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

void HAL_SpiInit(void)
{
    GLB_IO_ENABLE *pGLB68 = (GLB_IO_ENABLE *)(0x1ff80000 + 0x68);
    pGLB68->RSPI_NEW = 1;
    pGLB68->RNEWSPI_MODE = 2;
    pGLB68->RSPI_EN = 1;
    pGLB68->RSPI_DIS = 0;
    pGLB68->RSPI_CKG_EN = 0;

    HAL_DMAEInit();
    HAL_SpiHWInit();
    HAL_SpiSoftReset();
}
/******************************************************************************
FUNC: HAL_SpiDmaRead
Input:  ulDst  --  destination address
        ulSrc  --  source address (in SPI)
        ulLenB --  data request length ,count in byte
Output: 
Description:
    1. DMA read from SPI 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL void HAL_SpiDmaRead(U32 ulDst,U32 ulSrc,U32 ulLenB)
{
    HAL_DMAECopyOneBlock((const U32)ulDst,(const U32)ulSrc,ulLenB);    
}
/******************************************************************************
FUNC: HAL_SpiDmaPageWrite
Input:  ulDst  --  destination address (in SPI)
        ulSrc  --  source address 
        ulLenBLimit --  data request length ,count in byte ,Max value is 256
Output: SUCCESS / FAIL
Description:
    DMA page write into SPI, the accessable memory must belong to one page.
    
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL U32 HAL_SpiDmaPageWrite(U32 ulDst,U32 ulSrc,U32 ulLenBLimit)
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
FUNC: HAL_SpiBlockErase
Input:  ulOffset  --- Address to erase ,shall be block aligned
Output: SUCCESS / FAIL
Description:
    Erase a spi block (64K)
    
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL U32 HAL_SpiBlockErase(U32 ulOffset)
{
    U8 ucCmdType;    
    U8 ucCContByteLen;

    if (0 != (ulOffset & SPI_BLK_SIZE_MASK))
    {
        return FAIL;
    }

    if (SPI_ADDR_4B == l_pSpiRegSet->bsAddrMode)
    {
        ucCmdType = SPI_CMD_BE4B;
        ucCContByteLen = 4;
    }
    else
    {
        ucCmdType = SPI_CMD_BE_64;
        ucCContByteLen = 3;
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
    return SUCCESS;
}
/******************************************************************************
FUNC: HAL_SpiSecErase
Input:  ulOffset  --- Address to erase ,shall be sector aligned
Output: SUCCESS / FAIL
Description:
    Erase a spi sector (4K)
    
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL U32 HAL_SpiSecErase(U32 ulOffset)
{
    U8 ucCmdType;    
    U8 ucCContByteLen;

    if (0 != (ulOffset & SPI_SEC_SIZE_MASK))
    {
        return FAIL;
    }

    if (SPI_ADDR_4B == l_pSpiRegSet->bsAddrMode)
    {
        ucCmdType = SPI_CMD_SE4B;
        ucCContByteLen = 4;
    }
    else
    {
        ucCmdType = SPI_CMD_SE;
        ucCContByteLen = 3;
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
    return SUCCESS;
}


/************************ end of file ************************/

