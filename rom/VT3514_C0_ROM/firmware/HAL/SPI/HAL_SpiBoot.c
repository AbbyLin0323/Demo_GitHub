
#include "COM_Inc.h"

LOCAL volatile SPI_FLASH_FILE_HEAD l_tSpiRomHead;

void HAL_SpiBootProcess(void)
{
    DBG_TRACE(TRACE_SPI_BOOT);
    HAL_SpiInit();

    HAL_SpiDmaRead((U32)OTFB_START_ADDRESS,(U32)SPI_FLASH_FILE_HEAD_ADDR,sizeof(SPI_FLASH_FILE_HEAD));
    HAL_MemCpy((U32*)&l_tSpiRomHead,(U32*)OTFB_START_ADDRESS,sizeof(SPI_FLASH_FILE_HEAD)/sizeof(U32));

    if(l_tSpiRomHead.ulSignature != 0x3514)
    {
        return ;
    }

    HAL_SpiDmaRead(l_tSpiRomHead.ulTargetAddr,SPI_START_ADDRESS + l_tSpiRomHead.ulSegOffset,l_tSpiRomHead.ulSegLength);
    DBG_Printf("GET bootloader succ.\n");
    DBG_Printf("EXE ENTRY is 0x%x.\n",l_tSpiRomHead.ulExecEntry);
    ((PFUNC)l_tSpiRomHead.ulExecEntry)();
}

#if 0    // program spi image
void TEST_SpiImage(void)
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
}
#endif

