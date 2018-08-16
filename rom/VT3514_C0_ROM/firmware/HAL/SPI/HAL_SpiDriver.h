#ifndef HAL_SPI_DRIVER_H_
#define HAL_SPI_DRIVER_H_
#include "BaseDef.h"

#define SPI_CBUF_SIZE_BYTE  (8)
#define SPI_CBUF_SIZE_DW    (SPI_CBUF_SIZE_BYTE >> 2)

#define SPI_BLK_SIZE_BITS   (16)
#define SPI_BLK_SIZE        (1 << SPI_BLK_SIZE_BITS)
#define SPI_BLK_SIZE_MASK   (SPI_BLK_SIZE - 1)

#define SPI_SEC_SIZE_BITS   (12)
#define SPI_SEC_SIZE        (1 << SPI_SEC_SIZE_BITS)
#define SPI_SEC_SIZE_MASK   (SPI_SEC_SIZE - 1)

#define SPI_PAGE_SIZE_BITS   (8)
#define SPI_PAGE_SIZE        (1 << SPI_PAGE_SIZE_BITS)
#define SPI_PAGE_SIZE_MASK   (SPI_PAGE_SIZE - 1)

typedef struct _SPI_REG_SET_ 
{
    /* DW 0 : spi global setting & write command setting */
    U32 bsRes0_0      :3;
    U32 bsWCmdEn      :1; 
    U32 bsWSpiGen     :2;
    U32 bsWIOMode     :2;    
    U32 bsWCmdCode    :8;      
    U32 bsRes0_1      :7;
    U32 bsSnf         :1;     
    U32 bsSnfcCkEn    :1;      
    U32 bsHold        :1;
    U32 bsWp          :1;
    U32 bsAddrMode    :1;
    U32 bsSpiGen      :2;
    U32 bsSpiMode     :2;

    /* DW 1 : spi read command setting */
    U32 bsDummy       :8;
    U32 bsRCmdCode    :8;
    U32 bsCRMBT       :8;
    U32 bsRes1_0      :2;
    U32 bsCRMEN       :1;
    U32 bsRSpiGen     :2;
    U32 bsRIOMode     :2;
    U32 bsRCmdEn      :1;
    
    /* DW 2 : spi control command setting */
    U32 bsRes2_0      :7;       // all '0'
    U32 bsCRespValid  :1;       // 1 -- Receiption complete , 0 -- busy
    U32 bsSpiType     :2;       // for normal spi (spi device but not spi nor flash)
                                // 00 First cmd ,send cmd , to be continue
                                // 01 First cmd , send cmd ,wont continue 
                                // 10 continue read data without sending cmd
                                // 11 last time for continue read without sending cmd
    U32 bsAutoStp     :1;       // Auto stop after receiving enough data byte
    U32 bsCCmdValid   :1;       // Set for triggering cmd  
    U32 bsCRespNum    :4;       // Expect output data length

    U32 bsCLength     :4;       // Input data length
    U32 bsCSpiGen     :2;       // 00 SPI ,01 DPI,10 QPI , 11 S+Q
    U32 bsCIOMode     :2;       // bit[0] - ADDR ,bit [1]  - CMD
 
    U32 bsCCmdCode    :8;       // 1 byte command code 

    /* DW 3 & 4 : spi control command input data buf 64 byte */
    union 
    {
        U32 ulCContentDW[SPI_CBUF_SIZE_DW];
        U8  ucCContentByte[SPI_CBUF_SIZE_BYTE];
        struct
        {
            U32 ulAddress;
            U32 bsRes3;
        };
    };
    /* DW 5 & 6 : spi control command output data buf 64 byte*/
    union
    {
        U32 ulCRespDW[SPI_CBUF_SIZE_DW];
        U32 ucCRespByte[SPI_CBUF_SIZE_BYTE];
    };    
}SPI_REG_SET;


typedef enum __SNFC_PARAM__
{
    /* SPI mode */
    SPI_MODE = 0,
    QPI_MODE, 

    /* SPI Speed grade selection */
    SPI_DATAIO_1BIT = 0,
    SPI_DATAIO_2BIT,
    SPI_DATAIO_4BIT,

    /* Address mode selection */
    SPI_ADDR_3B = 0,
    SPI_ADDR_4B,  

    /* SPI IO MODE*/
    SPI_CMDIO_1BIT = 0,
    SPI_CMDIO_2BIT = 1,
    SPI_CMDIO_4BIT = 3,

}SNFC_PARAM__;


typedef struct  GLB_IO_ENABLE_
{
    /* byte 0 */
    U32 RGPIO_CKG_EN :1;
    U32 RLED0_EN     :1;
    U32 RLED1_EN     :1;
    U32 RPHYRDY_FW   :1;
    U32 RPHYRDY_SEL  :1;
    U32 RIIC_CKG_EN  :1;
    U32 RSPI_CKG_EN  :1;
    U32 RUART_CKG_EN :1;

    /* byte 1 */
    U32 RSPI_DIS     :1;
    U32 RIIC_EN      :1;
    U32 RGPIO_EN     :1;
    U32 RSPI_EN      :1;
    U32 RUART_EN     :1;
    U32 RJTAG_EN     :1;
    U32 RNEWSPI_MODE :2;

    /* byte 2~3 */
    U32 RSPI_NEW     :1;
    U32 RLED_EN      :1;
    U32 RES          :14;
}GLB_IO_ENABLE;

GLOBAL void HAL_SpiInit(void);
GLOBAL void HAL_SpiDmaRead(U32 ulDst,U32 ulSrc,U32 ulLenB);
GLOBAL U32 HAL_SpiDmaPageWrite(U32 ulDst,U32 ulSrc,U32 ulLenBLimit);
GLOBAL U32 HAL_SpiBlockErase(U32 ulBlkOff);
GLOBAL U32 HAL_SpiSecErase(U32 ulSecOff);
GLOBAL U8  HAL_SpiReadStatus(U8 ulRSRCode);
GLOBAL void HAL_SpiWriteStatus(U8 ulWSRCode,U32 ulData,U32 ulLenB);
GLOBAL U32 HAL_SpiReadId(void);
GLOBAL void HAL_SpiSetWCmd(U32 ulIndex);
GLOBAL void HAL_SpiSetRCmd(U32 ulIndex);
#endif 
