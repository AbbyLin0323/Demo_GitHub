/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_Init.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Sata interface function
Others      :
Modify      :
****************************************************************************/

#include "BaseDef.h"
#include "HAL_Inc.h"

void HAL_PowerOnInit(void)
{
    /*    
    wire    COMRESET_INT_SEL  = REG10[16];
    wire    LRESET_NEW_SEL    = REG10[17];
    //wire    SDCCMD_RST_SEL    = REG10[18];
    //wire    SDCDMA_RST_SEL    = REG10[19];
    wire    SDCBLKDMACMD_EN   = REG10[20]; 

    PMU_Regitser_Base: 0x1ff81f00
    REG10[16] : ASR OOB done, set second interrupt or not.
    REG10[17] : HW reset TP/Link when COMReset.
    REG10[20] : SDCBLKDMACMD_EN, HW will block SDC_DMAC when COMReset interrupt FW or Serror.
                need to clear HD_Err_clr(SDC_offset_0xCC:bit18).
    REG10[21] : 1 = FW_COMINIT_RDY ¡§¡è???¨¢?PMU register FW_COMINIT_RDY_SUS= REG10[22];
                0 = FW_COMINIT_RDY ¡§¡è???¨¢? SATA register 1ff80812[0]
    REG10[22] : need to set COMINIT_EN after COMreset interrupt.  Inform hardware to send COMINIT and continue OOB sequence

    REG10[23] : FW Decode enable
    REG10[25] : when FW ready, set 1, HW send COMINIT.  total switch for REG10[21]/REG10[22]

    SDC_BASE_ADDRESS  0x14[7] = 1 :receive error command fis, do not interrupt FW.
    SDC_BASE_ADDRESS  0x11[0] = 1 :receive error command fis, interrupt FW.

    */
    
   // *(volatile U32 *)(0x1ff81f00 + 0x10) |= 0x2f30000;//(1<<25)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<17)|(1<<16);

    // PMU_10[20] : SDCBLKDMACMD_EN
    *(volatile U32 *)(0x1ff81f00 + 0x10) |= 0x2e30000; //(1<<25)|(1<<23)|(1<<22)|(1<<21)         |(1<<17)|(1<<16);
    *(volatile U8 *)(SDC_BASE_ADDRESS + 0x11) |= 1;
    *(volatile U8 *)(SDC_BASE_ADDRESS + 0x14) |= (1<<7);
    
}

void HAL_UartInit(void)
{
    rTOP_REGGLB68 |= RUART_EN;

    uart_init();
}


void HAL_Init(U32 *pFreeDramBase,U32 *pFreeOTFBBase)
{
    //HAL_UartInit();
    
    HAL_SataInitialize();
    HAL_SataDsgInit();

    HAL_PowerOnInit();

}

/********************** FILE END ***************/

