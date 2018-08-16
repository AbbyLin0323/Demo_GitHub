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
Filename    : HAL_SataDSG.h
Version     : Ver 1.1
Author      : Gavin
Date        : 2014.12.11
Description : header file for Sata DSG driver
Others      : 
Modify      :
20141211    Gavin     add support for VT3514 C0 HW
*******************************************************************************/
#ifndef __HAL_SATA_DSG_H__
#define __HAL_SATA_DSG_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define DSG_TYPE_READ       0  // Type 0. dsg number: 0~63;
#define DSG_TYPE_WRITE      1  // Type 1. dsg number:64~127;

/* define sata total DSG number */
#define SATA_TOTAL_DSG_NUM  128

/* define sata read DSG number */
#define SATA_TYPE0_DSG_NUM  64

/*define sata write DSG number */
#define SATA_TYPE1_DSG_NUM  64

#define SATA_DSG_SIZE_DW    8
#define READ_DSG_NUM        64
#define WRITE_DSG_NUM       64

#define rSGESataMode            (*((volatile U32*)(REG_BASE_SGE + 0x1c)))
#define SGE_SATAMODE_ENABLE     1
#define rSataDsgReportWrite1    (*((volatile U32*)(REG_BASE_SGE+0x3c)))
#define rSataDsgReportRead1     (*((volatile U32*)(REG_BASE_SGE+0x40)))
#define SATA_DSG_VALID_BIT      BIT(31)

//register 0x90 ~ 0x9a is only supported in C0 HW
#define rSataDsgReportRead2     (*((volatile U32*)(REG_BASE_SGE+0x90)))
#define rSataDsgReportWrite2    (*((volatile U32*)(REG_BASE_SGE+0x94)))
#define rAvailableReadDsgCnt    (*((volatile U8*)(REG_BASE_SGE+0x98)))
#define rAvailableWriteDsgCnt   (*((volatile U8*)(REG_BASE_SGE+0x9a)))

typedef struct _SATA_DSG_REPORT 
{
    U32 bsSataDsgValue      : 1;
    U32 bsSataDsgWrIndex    : 10;
    U32 bsSataDsgWrEn       : 1;
    U32 bsRsv0              : 8;
    U32 bsSataDsgTrigger    : 1;
    U32 bsSataDsgId         : 10;
    U32 bsSataDsgValidEn    : 1;
}SATA_DSG_REPORT,*pSATA_DSG_REPORT;

typedef enum _FwAckHostEn
{
    FW_TRIGGER  = 0,
    HW_TRIGGER  = 1
}EFwAckHostEn;

typedef enum _XferEndIntEn
{
    SDC_NOT_INT_MCU = 0,
    SDC_INT_MCU     = 1
}EXferEndIntEn;

typedef enum _ProtSel
{
    PROT_PIO        = 0,
    PROT_DMA_FPDMA  = 1
}EProtSel;

typedef enum _StatusVld
{
    BIT_FALSE   = 0,
    BIT_TRUE    = 1
}EStatusVld;

typedef enum _StatusEn
{
    BIT_DISABLE = 0,
    BIT_ENABLE  = 1
}EStatusEn;

typedef enum _DataLocSel
{
    DATA_IN_DRAM = 0,
    DATA_IN_SRAM = 1
}EDataLocSel;

typedef enum _CacheStsLocSel
{
    CS_IN_SRAM = 0,
    CS_IN_DRAM = 1
}ECacheStsLocSel;

typedef struct _SATA_INFO {
    U32 HCmdSector      : 16;       /* transfer length for this HostCmd           */
    U32 AckHost         : 1;        /* 0: HW ack; 1:FW ack                        */
    U32 FinishIntEn     : 1;        /* HW interrupt MCU or not when EOT finish    */
    U32 KeySel          : 4;        /* Key select number                          */
    U32 EMEnable        : 1;        /* encryption or not                          */
    U32 NonData         : 1;        /* NonData Valid.                             */
    U32 CmdTag          : 5;              
    U32 WriteBit        : 1;        /* 0: Read Command; 1: Write Command          */
    U32 AutoActiveEn    : 1;        /* Reserved, not used right now; keep 0       */
    U32 DMAEnable       : 1;        /* 1: DMA/FPDMA command; 0: not D/F command   */
} SATA_INFO;

/* ATA protocol related info define */
typedef struct _ATA_PROT_INFO_ {
    U32 CmdXferSecCnt   : 16;       // Bit 0~15  /* if IsNonDataCmd == 0, 0 means 65536 sectors    */
    U32 FwAckHostEn     : 1;        // Bit 16    /* if enable, D2H/SDB FIS is sent by FW trig      */
    U32 XferEndIntEn    : 1;        // Bit 17    /* if enable, SDC send int after cmd data finish  */
    U32 EcpKeySel       : 4;        // Bit 18~21 /* only need 2 bits??                             */
    U32 EcpEn           : 1;        // Bit 22    /* encryption or not                              */
    U32 IsNonDataCmd    : 1;        // Bit 23    /* NonData. 1:Nondata command. 0:NCQ command      */
    U32 CmdTag          : 5;        // Bit 24
    U32 IsWriteCmd      : 1;        // Bit 29    /* 1: Write Command  0:Read Command               */
    U32 AutoActiveEn    : 1;        // Bit 30    /* FW should enable this feature when NCQ Write   */
    U32 ProtSel         : 1;        // Bit 31    /* 1 = DMA/FPDMA, 0 = PIO                         */
} ATA_PROT_INFO, *P_ATA_PROT_INFO;

/* Control info define */
typedef struct _XFER_CTRL_INFO_ {
    U32 BuffLen         : 8;        // Bit 0~7   // if set to 0, data buff size register indicate actual length
    U32 BuffOffset      : 8;        // Bit 8~15  /* Buffer Map Offset from the addr (sector)       */
    U32 BuffMapId       : 7;        // Bit 16~22
    U32 BuffMapEn       : 1;        // Bit 23    /* Update Buffermap Enable(when cmd done for WCMD)*/
    U32 Reserved        : 4;        // Bit 24
   // U32 CacheStsLocSel  : 1;        /* 0 = cache status in SRAM. 1 = in DRAM          */
    U32 CacheStsEn      : 1;        // Bit 28    /* 1: need update cache status.  0: Not need */
    U32 DataLocSel      : 1;        // Bit 29    /* 0 = data in DRAM; 1 = in SRAM */
    U32 DummyDataEn     : 1;        // Bit 30    /* if enable , SDC generate data itself instead of fetch data from DRAM */
    U32 Eot             : 1;        // Bit 31    /* 1 means last DSG in chain */
} XFER_CTRL_INFO, *P_XFER_CTRL_INFO;

/* SATA DSG format define */
typedef struct _SATA_DSG_ {
    /* DWORD 0 */
    union {
        ATA_PROT_INFO AtaProtInfo;
        U32 DW0;
    };

    /* DWORD 1 */
    union {
        XFER_CTRL_INFO XferCtrlInfo;
        U32 DW1;
    };

    /* DWORD 2 */
    union {
        struct {
            U32 NextDsgId   : 8;
            U32 CacheStsData: 8;
            U32 CmdLbaHigh  : 16;
        };
        U32 DW2;
    };

    /* DWORD 3 */
    union {
        struct {
            U32 CacheStsAddr    : 31;
            U32 CacheStsLocSel  : 1;
        };
        U32 DW3;
    };

    /* DWORD 4 */
    U32 DataAddr;
    
    /* DWORD 5 */
    U32 CmdLbaLow;
    
    U32 Rsv0;
    
    U32 Rsv1;
} SATA_DSG;

extern void HAL_SataDsgInit(void);

extern BOOL HAL_GetCurSataDsg(U16 *PDsgId, U8 Type);

extern void HAL_TriggerSataDsg( U8 Type);

extern void HAL_GetCurAndTriggerNextSataReadDsg(U16 *PDsgId);
extern void HAL_GetCurAndTriggerNextSataWriteDsg(U16 *PDsgId);
extern U32 HAL_GetSataDsgAddr(U16 DsgId);

extern void HAL_SetSataDsgValid(U16 usDsgId);

extern void HAL_SetFirstDSGID(U8 ucCmdTag, U8 ucDSGID);

extern U8 HAL_GetAvailableSataDsgCnt( U8 Type);

extern void HAL_ResetSataDSG(void);
#endif

