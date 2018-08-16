/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :  HAL_GLBReg.h                                         
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
*************************************************/
#ifndef _HAL_SSDGLB_H
#define _HAL_SSDGLB_H

#define rGLB_0 (*((volatile U32*)(REG_BASE_GLB + 0x0)))
#define rGLB_4 (*((volatile U32*)(REG_BASE_GLB + 0x4)))
#define rGLB_8 (*((volatile U32*)(REG_BASE_GLB + 0x8)))
#define rGLB_C (*((volatile U32*)(REG_BASE_GLB + 0xC)))
#define rGLB_10 (*((volatile U32*)(REG_BASE_GLB + 0x10)))
#define rGLB_14 (*((volatile U32*)(REG_BASE_GLB + 0x14)))
#define GLB14_NFCLK_SEL     24
#define GLB14_NFCLK_SEL_MSK 0xF
#define GLB14_NFCLK_SEL_01X1 0x5

#define rGLB_18 (*((volatile U32*)(REG_BASE_GLB + 0x18)))
#define R_RST_REG       (1<<1)
#define R_RST_SDC_ICB   (1<<2)
#define R_RST_NFG       (1<<3)
#define R_RST_NF0       (1<<4)
#define R_RST_NF1       (1<<5)
#define R_RST_NF2       (1<<6)
#define R_RST_NF3       (1<<7)
#define R_RST_SDC       (1<<8)
#define R_RST_SDC_CMD   (1<<9)
#define R_RST_SDC_DMA   (1<<10)
#define R_RST_LPHY      (1<<11)
#define R_RST_SE1       (1<<12)
#define R_RST_SE0       (1<<13)
#define R_RST_DMAE      (1<<14)
#define R_RST_BUFM      (1<<15)
#define R_RST_TRFC      (1<<16)
#define R_RST_OTFB      (1<<17)
#define R_RST_APB       (1<<18)
#define R_RST_DRAMC     (1<<19)
#define R_RST_SGE       (1<<22)
#define R_RST_HCT       (1<<23)
#define R_RST_SNFC      (1<<24)
#define R_RST_HOSTC     (1<<25)
#define R_RST_EM        (1<<26)
#define R_RST_OTFBM     (1<<27)
#define R_RST_LDPC      (1<<28)
#define R_RST_PCIE      (1<<29)
#define R_RST_COMP      (1<<30)
#define R_RST_EPHY_RF   (1<<31)


#define rGLB_1C (*((volatile U32*)(REG_BASE_GLB + 0x1C)))
#define R_RST_MCU1      (1<<0)
#define R_RST_MCU1IF     (1<<1)
#define R_RST_MCU2      (1<<2)
#define R_RST_MCU2IF     (1<<3)
#define R_RST_MCU0      (1<<29)
#define R_RST_MCU0IF     (1<<30)
#define R_RST_MCUIF      (1<<31)


#define rGLB_20 (*((volatile U32*)(REG_BASE_GLB + 0x20)))
#define rGLB_24 (*((volatile U32*)(REG_BASE_GLB + 0x24)))
#define rGLB_28 (*((volatile U32*)(REG_BASE_GLB + 0x28)))
#define rGLB_2C (*((volatile U32*)(REG_BASE_GLB + 0x2C)))
#define rGLB_30 (*((volatile U32*)(REG_BASE_GLB + 0x30)))
#define rGLB_34 (*((volatile U32*)(REG_BASE_GLB + 0x34)))
#define rGLB_38 (*((volatile U32*)(REG_BASE_GLB + 0x38)))
#define rGLB_3C (*((volatile U32*)(REG_BASE_GLB + 0x3C)))
#define rGLB_40 (*((volatile U32*)(REG_BASE_GLB + 0x40)))
#define GLB40_RSRAMBOOT_MCU0 (1<<4)
#define GLB40_RDRAMBOOT_MCU0 (1<<5)
#define GLB40_RSRAMBOOT_MCU1 (1<<6)
#define GLB40_RDRAMBOOT_MCU1 (1<<7)
#define GLB40_RNC_SRAM (1<<24)
#define GLB40_RSE0_SRAM (1<<25)
#define GLB40_RSC_SRAM (1<<26)
#define GLB40_RSE1_SRAM (1<<27)
#define rGLB_40 (*((volatile U32*)(REG_BASE_GLB + 0x40)))
#define rGLB_44 (*(volatile U32*)(REG_BASE_GLB + 0x44))

#endif
/********************** FILE END ***************/

