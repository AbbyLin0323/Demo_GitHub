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
Filename     :  HAL_GLBReg.h
Version      :  Ver 1.0
Date         :
Author       :  PeterXiu

Description: Definitions for address of top global control registers.

Modification History:
20120118     peterxiu     001 first create
20150304     gavinyin     002 refine definition in this file
*******************************************************************************/
#ifndef _HAL_SSDGLB_H
#define _HAL_SSDGLB_H
#include "HAL_MemoryMap.h"

//global clock gating / clock selection control register
#define rGlbClkCtrl     (rGLB(0x4))
#define HCLK_DIV_SEL    (1 << 26)

//level 1 clock gating enable register
#define rGlbClkGating   (rGLB(0x14))

#define MSK_EN_SE0_CLK  (0x1 << 20)
#define MSK_EN_SE1_CLK  (0x1 << 21)
#define MSK_EN_SE2_CLK  (0x1 << 25)

#define MSK_EN_DMAE_CLK (0x1 << 17)

//HW module software reset register
#define rGlbSoftRst     (rGLB(0x18))

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
#define R_RST_SE2       (1<<21)
#define R_RST_SGE       (1<<22)
#define R_RST_HCT       (1<<23)
#define R_RST_SNFC      (1<<24)
#define R_RST_HOSTC     (1<<25)
#define R_RST_EM        (1<<26)
#define R_RST_OTFBM     (1<<27)
#define R_RST_LDPC      (1<<28)
#define R_RST_PCIE      (1<<29)
#define R_RST_COMP      (1<<30)
#define R_RST_EPHY      (1<<31)

//MCU/SGE(C0) reset register
#define rGlbMcuSgeRst    (rGLB(0x1c))

#define R_RST_MCU1          (1<<0)
#define R_RST_MCU1IF        (1<<1)
#define R_RST_MCU2          (1<<2)
#define R_RST_MCU2IF        (1<<3)
#define R_RST_SGEHSG        (1<<16)
#define R_RST_SGEDSG        (1<<17)
#define R_RST_SGECFG        (1<<18)
#define R_RST_SGECHNCNT     (1<<19)
#define R_RST_XORE          (1<<20)
#define R_RST_HGE           (1<<21)
#define R_RST_DAA           (1<<22)
#define R_RST_HUPARB        (1<<23)
#define R_RST_HOSTCRC       (1<<24)
#define R_RST_HOSTC_DN      (1<<26)
#define R_RST_HOSTC_MMCFG   (1<<27)
#define R_RST_HOSTC_GNRLCFG (1<<28)
#define R_RST_MCU0          (1<<29)
#define R_RST_MCU0IF        (1<<30)
#define R_RST_MCUIF         (1<<31)

//MCU lcoal SRAM mapping
#define rGlbMCUSramMap   (rGLB(0x24))

//OTFB module register
#define rGlbOTFBMCtrl0   (rGLB(0x28))
#define rGlbOTFBMCtrl1   (rGLB(0x2c))

//MCU misc register
#define rGlbMCUMisc      (rGLB(0x40))

//MCU boot/stall/halt mode register
#define rGlbMCUCtrl      (rGLB(0x44))
#define RMCU1_USE_ALTRESETVECT  (1 << 4)
#define RMCU1_EXEC_STALL        (1 << 5)
#define RMCU2_USE_ALTRESETVECT  (1 << 8)
#define RMCU2_EXEC_STALL        (1 << 9)

//HW debug signal reigster
#define rGlbDbgSigSel    (rGLB(0x54))

//TRFC related reigster
#define rGlbTrfc         (rGLB(0x64))

#define MSK_EN_SE0_OTFB  (0x1 << 5)
#define MSK_EN_SE1_OTFB  (0x1 << 6)
#define MSK_EN_SE2_OTFB  (0x1 << 7)

//register for debug/trace usage in simualtion(COSIM/XTMP/VS) ENV
#define TRACE_REG_BASE REG_BASE_GLB
#define rTracer     (*((volatile U32*)(TRACE_REG_BASE+0x80)))
#define rTraceAddr1 (*((volatile U32*)(TRACE_REG_BASE+0x84)))
#define rTraceAddr2 (*((volatile U32*)(TRACE_REG_BASE+0x88)))
#define rTraceData0 (*((volatile U32*)(TRACE_REG_BASE+0x8c)))

#define rTraceData1 (*((volatile U32*)(TRACE_REG_BASE+0x90)))
#define rTraceData2 (*((volatile U32*)(TRACE_REG_BASE+0x94)))
#define rTraceData3 (*((volatile U32*)(TRACE_REG_BASE+0x98)))
#define rTraceData4 (*((volatile U32*)(TRACE_REG_BASE+0x9c)))

#define rTraceData5 (*((volatile U32*)(TRACE_REG_BASE+0xa0)))
#define rTraceData6 (*((volatile U32*)(TRACE_REG_BASE+0xa4)))
#define rTraceData7 (*((volatile U32*)(TRACE_REG_BASE+0xa8)))
#define rTraceData8 (*((volatile U32*)(TRACE_REG_BASE+0xac)))

#define rTraceMonitorAddr (*((volatile U32*)(TRACE_REG_BASE+0xb0)))
#define rTraceMonitorValue (*((volatile U32*)(TRACE_REG_BASE+0xb4)))
#define rTraceMointorStart (*((volatile U32*)(TRACE_REG_BASE+0xb8)))

/*  add by abby for NFC test info tracer
2 DWs, 0x1ff80080~0088                  */
typedef union _TEST_NFC_TRACE_INFO_
{
    U32 ulTraceDw[2];
    struct
    {
        //DW0
        U32 bsDone      : 1;    //1-Pattern test done; 0-not finish
        U32 bsFail      : 1;    //1-Current cmd fail or error; 0-success
        U32 bsDataErr   : 1;    //1-data check fail
        U32 bsRsv0      : 5;    //align in 1 Byte
        U32 bsReqType   : 8;
        U32 bsLun       : 8;
        U32 bsPu        : 8;

        //DW1
        U32 bsPage      : 16;
        U32 bsBlk       : 16;
    };
}TEST_NFC_TRACE_INFO;

typedef volatile union _OTFB_MODEL_CTRL_REG0_
{
    U32 ulOtfbModelCtrl0DW0;
    struct
    {
        volatile U32 ulRsv              : 5;
        volatile U32 bROtfbDbgSel       : 3;    //OFTB Debug Signal Group Sel
        volatile U32 bROtfbHostPriori   : 1;    //When Host and NF both access one SRAM, Host is priority. Hign valid
        volatile U32 bROtfbHostHoldNfc  : 1;    //When Host and NF both access one SRAM, NFC part will not do command. Hign valid.
        volatile U32 bROtfbHostHoldNfcCjhold : 1;   //When Host and NF both access one SRAM,HostC Hold
        volatile U32 bROtfbmDbgSel      : 4;    //OTFBM debug sel
        volatile U32 ulRsv2             : 1;
        volatile U32 bROtfbmPmen        : 1;    //OFTBM Model clk gating enable, high valid.
        volatile U32 bROtfbBfuNum       : 1;    //The number of 8k buffer per NF channel. 1'b0:4(32k),1'b1:8(64k)
        volatile U32 ulRsv3             : 2;    //Rotfb_SELFCLR_EN?? Fix Me.
        volatile U32 ulROtfbmCh332kSel0 : 5;    //SRAM Index allocated for NF ch3 when AHCI Mode.
        volatile U32 ulROtfbmCh332kSel1 : 5;    //SRAM Index allocated for NF ch3 when AHCI Mode.
        volatile U32 ulRsv4             : 2;
    };
}OTFB_MODEL_CTRL_REG0, *POTFB_MODEL_CTRL_REG0;

typedef volatile union _OTFB_MODEL_CTRL_REG1_
{
    U32 ulOtfbModelCtrl1DW0;
    struct
    {
        volatile U32 ulROtfbmCh032kSel0 : 5;  //SRAM Index allocated for NF ch0 when AHCI Mode.
        volatile U32 ulROtfbmCh032kSel1 : 5;  //SRAM Index allocated for NF ch0 when AHCI Mode.
        volatile U32 ulROtfbmCh132kSel0 : 5;  //SRAM Index allocated for NF ch1 when AHCI Mode.
        volatile U32 ulROtfbmCh132kSel1 : 5;  //SRAM Index allocated for NF ch1 when AHCI Mode.
        volatile U32 ulROtfbmCh232kSel0 : 5;  //SRAM Index allocated for NF ch2 when AHCI Mode.
        volatile U32 ulROtfbmCh232kSel1 : 5;  //SRAM Index allocated for NF ch2 when AHCI Mode.
    };
}OTFB_MODEL_CTRL_REG1, *POTFB_MODEL_CTRL_REG1;

#endif

/********************** FILE END ***************/

