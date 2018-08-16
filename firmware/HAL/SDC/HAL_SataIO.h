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
Filename    :HAL_SataIO.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    17:20:35
Description :
Others      :
Modify      :
*******************************************************************************/

#ifndef _HAL_SATAIO_H
#define _HAL_SATAIO_H

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define FIS_Delay

/***********************************************
SDC/SACMDM/SDMAC Registers Address Definition
************************************************/
#define SATA_BASE_ADDRESS       REG_BASE_SDC

//SATA device controller related registers
#define SDC_BASE_ADDRESS        (SATA_BASE_ADDRESS + 0x0)

//PHY control registers
#define SATA_PHYCONFIG_REGISTER_BASE    (SDC_BASE_ADDRESS + 0x0)
#define rSDC_PHYReg32           ((volatile U32 *)SATA_PHYCONFIG_REGISTER_BASE)
#define rSDC_PHYTestModeCtrl1   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x00))
#define rSDC_PHYTestModeCtrl2   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x01))
#define rSDC_PHYTestModeCtrl3   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x02))
#define rSDC_PHYControl1        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x03))
#define rSDC_PHYControl2        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x04))
#define rSDC_PHYControl3        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x05))
#define CLR_NCQ_CMD_EN          (1 << 3)
#define EN_STOP_ASR             (1 << 7)
#define rSDC_PHYControl4        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x06))

#define rSDC_PHYControl5        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x07))
#define ASR_SUPPORT_EN          (1 << 0)

#define rSDC_PHYControl6        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x08))
#define INC_OOB_BURST           (1 << 4)
#define rSDC_PHYControl7        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0A))
#define rSDC_PHYControl8        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0B))
#define rSDC_PHYControl9        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0C))
#define rSDC_PHYControl10       (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0D))
#define rSDC_PHYControl11       (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0E))

// Shadow register block firmware programming lock control
#define rSDC_SHR_LockControl        (*(volatile U8 *)(SDC_BASE_ADDRESS + 0x0F))    //VT3492->VT3514: 0x31->0x0F
#define BIT_SDC_SHR_PROG_EN         (1<<3)
#define BIT_FW_CMDINT_EN            (1<<1)  /* if en, HW will interrupt FW when NCQ cmd Data xfer finished */
#define BIT_SOFTWARE_RST_SUPPORT    (1<<2)

#define rSDC_PHYControl13       (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x10))

#define rSDC_PHYControl14       (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x11))
#define R_ERR_SRC_SEL           (1 << 0)
#define EPHY_IDLE_DELAY_SEL     (1 << 1)

/*[0]-FW is ready and inform HW to send com_init to host*
 *[1]-FW clear the FW trigger register block signal    */
#define rSDC_PHYControl15       (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x12))
#define FW_COMINIT_RDY_TRG      (1<<0)
#define FW_CLR_COMRESET_BLOCK   (1<<1)

#define rSDC_LKControl          (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x14))
#define LK_ROK_DELAY_SEL        (1 << 7)

#define rSDC_AtaCFisDW0         (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x44))
#define rSDC_AtaCFisDW1         (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x48))
#define rSDC_AtaCFisDW2         (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x4C))
#define rSDC_AtaCFisDW3         (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x30))
#define rSDC_AtaCFisDW4         (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x34))

#define rSDC_AFEControl3        (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x09))
#define rSDC_AFEControl54       (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x47))
//After FW set corresponding to 1, SDC can reply D2H(for DMA) or SDB(for NCQ) to complete commnad
#define rSDC_SendSDBFISReady    (*(volatile U32 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x54))

//Link and command status control
#define rSDC_LinkStatus         (*(volatile U16 *)(SDC_BASE_ADDRESS + 0x70))
#define DPM_PWR_STATE_NO_COMM       0x00
#define DPM_PWR_STATE_IF_IN_ACTIVE  0x01
#define DPM_PWR_STATE_IF_IN_PARTIAL 0x02
#define DPM_PWR_STATE_IF_IN_SLUMBER 0x06

#define rSDC_ErrorStatus        (*(volatile U16 *)(SDC_BASE_ADDRESS + 0x74))
#define BIT_SDC_ERROR_STATUS_RXRDIE (1<<0)  // Device Received Invalid REG H2D FIS
#define BIT_SDC_ERROR_STATUS_RXNRPE (1<<8)  // Device sent Non-Data Fis 3 times with R_ERR terminated
#define BIT_SDC_ERROR_STATUS_RXNRTE (1<<9)  // Device sent Data Fis with R_ERR terminated
#define BIT_SDC_ERROR_STATUS_RXPCLE (1<<10) // Host sync-escaped device-sent FIS
#define BIT_SDC_ERROR_STATUS_RXINLE (1<<11) // Device received FIS with Unknown Type
#define BIT_SDC_ERROR_STATUS_RXNRDE (1<<12) // Device received Data Fis with CRC error

#define rSDC_LinkControl        (*(volatile U16 *)(SDC_BASE_ADDRESS + 0x78))
#define rSDC_NCQActive          (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x7C))
#define rSDC_NCQOutstd          (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x80))

//Shadow block registers
#define SATA_COMMAND_BLOCK_REGISTER_BASE    (SDC_BASE_ADDRESS + 0x84)

#define rSDC_DATA               (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 0))
#define rSDC_FEATURE_ERROR      (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 1))
#define BIT_SDC_FEATURE_ERROR_MED   (1<<0)
#define BIT_SDC_FEATURE_ERROR_NM    (1<<1)
#define BIT_SDC_FEATURE_ERROR_ABRT  (1<<2)
#define BIT_SDC_FEATURE_ERROR_MCR   (1<<3)
#define BIT_SDC_FEATURE_ERROR_IDNF  (1<<4)
#define BIT_SDC_FEATURE_ERROR_MC    (1<<5)
#define BIT_SDC_FEATURE_ERROR_WP    (1<<6)
#define BIT_SDC_FEATURE_ERROR_ICRC  (1<<7)

#define rSDC_SECCNT         (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 2))
#define rSDC_LBALOW         (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 3))
#define rSDC_LBAMID         (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 4))
#define rSDC_LBAHIGH        (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 5))
#define rSDC_DEVICE_HEAD    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 6))
#define BIT_SDC_DEV_LBA_ENABLE      (1 << 6)
#define rSDC_COMMAND_STATUS (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 7))
#define BIT_SDC_COMMAND_STATUS_ERR  (1<<0)
#define BIT_SDC_COMMAND_STATUS_DRQ  (1<<3)
#define BIT_SDC_COMMAND_STATUS_CD   (1<<4)
#define BIT_SDC_COMMAND_STATUS_DF   (1<<5)
#define BIT_SDC_COMMAND_STATUS_DRDY (1<<6)
#define BIT_SDC_COMMAND_STATUS_BSY  (1<<7)

#define SATA_CONTROL_BLOCK_REGISTER_BASE        (SDC_BASE_ADDRESS + 0x8C)
#define rSDC_CONTROL_ALTSTATUS                  (*(volatile U8 *)(SATA_CONTROL_BLOCK_REGISTER_BASE + 0))
#define BIT_SDC_CONTROL_SRST        (1 << 2)

#define SATA_EXP_COMMAND_BLOCK_REGISTER_BASE    (SDC_BASE_ADDRESS + 0x90)
#define rSDC_EXP_FEATURE        (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 1))
#define rSDC_EXP_SECCNT         (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 2))
#define rSDC_EXP_LBALOW         (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 3))
#define rSDC_EXP_LBAMID         (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 4))
#define rSDC_EXP_LBAHIGH        (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 5))
#define rSDC_END_STATUS         (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 7))

//Parameters for D2H FIS construction
#define SATA_COMMAND_PARAMETER_REGISTER_BASE    (SDC_BASE_ADDRESS + 0x98)
#define rSDC_PTDef              (*(volatile U8 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 0))
#define rSDC_PIOXferCountLow    (*(volatile U8*)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 1))
#define rSDC_PIOXferCountHigh   (*(volatile U8*)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 2))
#define rSDC_FISDirInt          (*(volatile U8 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 19)) //[20090701]JackeyChai: can not trigger a access cross DWORD boundary
#define BIT_SDC_FIS_DIRFLAG     (1 << 0)
#define BIT_SDC_FIS_INTFLAG     (1 << 1)

//Miscellaous SDC registers
#define rSDC_IOControl          *(volatile U8 *)(SDC_BASE_ADDRESS + 0xAC)
#define BIT_SDC_IOCTRL_SENDBISTACT_FIS  (1 << 0)
#define BIT_SDC_IOCTRL_SENDSDB_FIS      (1 << 1)
#define BIT_SDC_IOCTRL_SENDDMADATA_FIS  (1 << 2)
#define BIT_SDC_IOCTRL_SENDPIODATA_FIS  (1 << 3)
#define BIT_SDC_IOCTRL_SENDDMASETUP_FIS (1 << 4)
#define BIT_SDC_IOCTRL_SENDDMAACT_FIS   (1 << 5)
#define BIT_SDC_IOCTRL_SENDPIOSETUP_FIS (1 << 6)
#define BIT_SDC_IOCTRL_SENDREGD2H_FIS   (1 << 7)

#define rSDC_IntSrcPending      *(volatile U32 *)(SDC_BASE_ADDRESS + 0xB0)
#define BIT_SDC_INTSRC_EOTPENDING       (1 << 0)
#define BIT_SDC_INTSRC_LPM              (1 << 1)
#define BIT_SDC_INTSRC_FIFOFULL         (1 << 2)
#define BIT_SDC_INTSRC_SERROR           (1 << 3)
#define BIT_SDC_INTSRC_SOFTRESET        (1 << 4)
#define BIT_SDC_INTSRC_TXOP             (1 << 7)
#define BIT_SDC_INTSRC_NCQERR           (1 << 8)
#define BIT_SDC_INTSRC_NCQFINISH        (1 << 9)
#define BIT_SDC_INTSRC_FIS_CONTROL      (1 << 11)
#define BIT_SDC_INTSRC_FIS_COMMAND      (1 << 12)
#define BIT_SDC_INTSRC_OOB_DONE         (1 << 13)
#define BIT_SDC_INTSRC_RXPIO_DATA       (1 << 14)
#define BIT_SDC_INTSRC_RXDMACMPL        (1 << 15)
#define BIT_SDC_INTSRC_COMRESET_RCV     (1 << 16)
#define BIT_SDC_INTSRC_PENDING          (1 << 17)
#define BIT_SDC_INTSRC_SOFTRESET_FIRST  (1 << 18)
#define BIT_SDC_INTSRC_BISTRCV          (1 << 19)
#define BIT_SDC_INTSRC_INFER_EIDLE      (1 << 20)

#define rSDC_NCQCMD_CLR_TAG     *(volatile U32 *)(SDC_BASE_ADDRESS + 0xB4)
#define rSDC_NCQCMD_HOLD_TAG    *(volatile U32 *)(SDC_BASE_ADDRESS + 0xBC)

#define rSDC_IntMask            (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xB8))

#define rSDC_FISDelayControl    (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xC0))
#define TXSETDEVCOMPL_DLY_EN    (1<<31)

#define rSDC_PwrAndCmdStatus    (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xC8))
#define SDC_ALL_CMD_DONE        (1 << 16)
#define SDC_PIO_CMD_PENDING     (1 << 15)
#define SDC_DMA_CMD_PENDING     (1 << 14)
#define SDC_ALL_NCQ_CMD_DONE    (1 << 13)

#define SDC_TXPWR_NORMAL   0
#define SDC_TXPWR_SLUMBER  1
#define SDC_TXPWR_PARTIAL  2

#define rSDC_ControlRegister    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xCC))
#define FW_DMAEXE_STE_EN    (1<<6)
#define PARTSLU_EN          (1<<5)
#define HW_COMWAKE_EN       (1<<4)
#define HW_SLUMBER_EN       (1<<3)
#define HW_PARTIAL_EN       (1<<2)
#define DIPM_EN             (1<<1)
#define HW_PWR_EN           (1<<0)

/*[0:2]-FW configure current command type,100-PIO,101-DMA,110,NCQ *
* [7:3] - FW configure current command tag                        */
#define rSDC_FW_DECODE      (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xCD))
/*[0]-If is not a NCQ cmd, FW should trigger to idle before clear the interrupt          *
* [1]-When FW receive and analysis the cmd ,should trigger this signal to clear big busy *
* [2]-When encounter a error, hw will hold DMA and CMD module , FW will clear the HD_Err *
* [3]-When PIO /NONDATA cmd is finish ,FW trigger HW to clear small busy                 */
#define rSDC_FW_Ctrl        (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xCE))
#define FW_CLR_NCQEXE       (1)
#define FW_CFGCMD_DONE      (1<<1)  //big busy
#define HD_Err_clr          (1<<2)
#define CLR_PIOCMD_DATA     (1<<3)  //small busy
#define rSDC_FW_Ctrl16      (*(volatile U16 *)(SDC_BASE_ADDRESS + 0xCE))


#define rSDC_SHRLCH_LBA28       (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xD0))    //34
#define rSDC_SHRLCH_LBA48       (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xD4))
#define rSDC_SHRLCH_SECCNT8     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD8))
#define rSDC_SHRLCH_SECCNT16    (*(volatile U16 *)(SDC_BASE_ADDRESS + 0xD8))
#define rSDC_SHRLCH_FEATURE8    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDA))
#define rSDC_SHRLCH_FEATURE16   (*(volatile U16 *)(SDC_BASE_ADDRESS + 0xDA))
#define rSDC_SHRLCH_COMMAND     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDC))
#define rSDC_SHRLCH_NCQTAG      (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDE))
#define rSDC_SHRLCH_EN          (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE0))
#define rSDC_SHRLCH_CONTROL     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE1))
#define rSDC_SHRLCH_DEV6        (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE2))
#define rSDC_SHRLCH_AUXILIARY   (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x40))    /* VT3514 add to support sata 3.2 */
#define rSDC_SHRLCH_ICC         (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE7))     /* VT3514 add to support sata 3.2 */

#define rSDC_LATH_FEATURE_ERROR     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDA))
#define rSDC_LATH_SECCNT            (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD8))
#define rSDC_LATH_LBALOW            (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD0))
#define rSDC_LATH_LBAMID            (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD1))
#define rSDC_LATH_LBAHIGH           (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD2))
#define rSDC_LATH_DEVICE_HEAD       (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDD))
#define rSDC_LATH_COMMAND_STATUS    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDC))
#define rSDC_LATH_CONTROL_ALTSTATUS (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE1))
#define rSDC_LATH_EXP_FEATURE       (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDB))
#define rSDC_LATH_EXP_SECCNT        (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD9))
#define rSDC_LATH_EXP_LBALOW        (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE4))
#define rSDC_LATH_EXP_LBAMID        (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE5))
#define rSDC_LATH_EXP_LBAHIGH       (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE6))
#define rSDC_LATH_EXP_STATUS        (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE3))

//SATA command management controller related registers
#define SACMDM_BASE_ADDRESS     (SATA_BASE_ADDRESS + 0x100)

/* first DSG id register, every command tag owns one U8 register
    Usage: rFIRST_DSG_ID[tag] = dsg id
*/
#define rFIRST_DSG_ID           ((volatile U8 *)SACMDM_BASE_ADDRESS)

//SATA main storage DMA controller related registers
#define SDMAC_BASE_ADDRESS      (SATA_BASE_ADDRESS + 0x300)

#define rSDMAC_DataBuffBase     (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x0))
//BUFFSIZE[2:0] 0:4K, 1:8K, 2:16K, 3:32K, 4:64K, 5:128K 
#define BUFFSIZE_4K     0
#define BUFFSIZE_8K     1
#define BUFFSIZE_16K    2
#define BUFFSIZE_32K    3
#define BUFFSIZE_64K    4
#define BUFFSIZE_128K   5
/* 2 ^ 12 = 4K*/
#define SIZE_4K_BITS    12

#define DUMMYDATA       (1<<3)
#define SCRUMBLE        (1<<4)
#define SDBFISwDMA      (1<<7)
#define GLOBALERROR     (1<<8)
#define HOLDXFER        (1<<9)  /* PRD hold DMA */
#define BYPASSEN        (1<<10)


#define rSDMAC_ScrambleKey      (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x4))
#define rSDMAC_PRDAttribute     (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x10))
#define rSDMAC_PRDMemAddr       (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x14))
#define rSDMAC_PRDXferLen       (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x18))
#define rSDMAC_PRDBuffMapID     (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x1C))
#define rSDMAC_ConfigReg4       (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x20))
#define rSDMAC_PRDLBAHigh       (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x24))
#define rSDMAC_Status           (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x30))
#define rSDMAC_CurBufMapId      (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x34))

#define rSDMAC_ErrorHandling    (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x40))
#define rSDMAC_CmdType          (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x44))

//PIO protocol handler state
#define SATA_PIO_NOCMD      0x00
#define SATA_PIO_NEWCMD     0x01
#define SATA_PIO_SETUP      0x02
#define SATA_PIO_DATA_IN    0x03
#define SATA_PIO_DATA_OUT   0x04
#define SATA_PIO_FINISH     0x05

#define PROTOCAL_SELECT(Protocol)   ((HCMD_PROTOCOL_PIO == (Protocol))?PROT_PIO:PROT_DMA_FPDMA)

/* Enable/Disable Firmware to Configure ERROR and STATUS in Shadow Register */
#define LOCK_SHADOW_REG()   (rSDC_SHR_LockControl |= BIT_SDC_SHR_PROG_EN)
#define UNLOCK_SHADOW_REG() (rSDC_SHR_LockControl &= ~BIT_SDC_SHR_PROG_EN)

/* PRD structures */
typedef struct _ATAPROTINFO {
    U32 usCmdXferLenSect    : 16;
    U32 ucBuffMapID         : 6;
    U32 bFirstPRDFlag       : 1;
    U32 bXferDirct          : 1;    // 1 = write command
    U32 ucCmdTag            : 5;
    U32 bDMASetupInt        : 1;
    U32 uResv1              : 1;
    U32 bHWProtlEn          : 1;    // 0 = for PIO command
} ATAPROTINFO, *PATAPROTINFO;

typedef struct _SDMAXFERINFO {
    U32 ucPRDXferLenSect    : 8;
    U32 ucBuffAddrHSect     : 8;
    U32 uResv2              : 5;
    U32 bCachStaLocSel      : 1;    // 1 = SRAM
    U32 bUpdWrBuffMapEn     : 1;
    U32 bBuffLocSel         : 1;    // 1 = SRAM(OTFB)
    U32 bXferEndIntEn       : 1;
    U32 bAutoAckHostEn      : 1;
    U32 ucEcpKeySel         : 2;
    U32 bEcpEn              : 1;
    U32 bStuffDataEn        : 1;
    U32 uResv3              : 1;
    U32 bXferEndFlg         : 1;
} SDMAXFERINFO, *PSDMAXFERINFO;

typedef struct _SGLENTRY {
    U32 ucBuffMapStartSect  : 8;
    U32 ucXferLenSect       : 8;
    U32 usBuffAddrLSect     : 16;
} SGLENTRY, *PSGLENTRY;

typedef struct _CACHSTAINFO {
    U32 usUpdAddr   : 16;
    U32 ucUpdValue  : 8;
    U32 bUpdEn      : 1;
    U32 uResv       : 7;
} CACHSTAINFO, *PCACHSTAINFO;

typedef enum _SATA_PRD_SPLIT_SIZE_
{
    PRD_SPLIT_SIZE_4K = 0,
    PRD_SPLIT_SIZE_8K,
    PRD_SPLIT_SIZE_16K,
    PRD_SPLIT_SIZE_32K,
    PRD_SPLIT_SIZE_64K,
    PRD_SPLIT_SIZE_128K
}SATA_PRD_SPLIT_SIZE;

//in FW decode mode, FW tells SDC the transfer protocol of a new command
typedef enum _SDC_PROTOCOL_
{
    SDC_PROTOCOL_PIO = 4,  //to match VT3514 hw design, start from 4.
    SDC_PROTOCOL_DMA = 5,
    SDC_PROTOCOL_NCQ = 6,
    SDC_PROTOCOL_NONDATA = 7,
}SDC_PROTOCOL;

void HAL_SataEnableNcqFinishInt(void);
void HAL_SataDisableNcqFinishInt(void);
void HAL_SataEnableCmdRcvInt(U32 ulEnable);
void HAL_SataInitialize(U8 ucBuffSizeBits);
void HAL_SDCResetDmaAndCmd(void);
void HAL_SDCHoldDmaAndCmd(void);
void HAL_SDCReleaseDmaAndCmd(void);
void HAL_SDCResetICB(void) ;
BOOL HAL_SataIsSdcIdle(void);
void HAL_SataRejectCommandOnReceiving(U8 ucCmdTag);
BOOL HAL_SataIsSlumber(void);
U32 HAL_GetNcqOutStandingCmd(void);
void HAL_SataClearAllNcqOutstd(void);
BOOL HAL_SataIsDmaBusy(void);
void HAL_SataClearBigBusy(void);
BOOL HAL_SataIsFISXferAvailable(void);
void HAL_SataSendSuccessStatus(void);
void HAL_SataSendAbortStatus(void);
void HAL_SataClearHWHoldErr(void);
void HAL_SataClearComResetBlock(void);
void HAL_SataSetAllSendSDBFISReady(void);
void HAL_SataClearAllSendSDBFISReady(void);
void HAL_SataClearSendSDBFISReady(U8 ucCmdTag);
void HAL_SataSendSetDevBitFIS(void);
void HAL_SataSendRegD2HFIS(void);
void HAL_SataSendPIOSetupFIS(void);
void HAL_SataSendPIODataFIS(void);
void HAL_SataSendGoodStatus(void);
void HAL_SataSendSDBUncorrectableError(void);
void HAL_SataSendSDBQueueCleanACT(void);
void HAL_SataGetOrigCFIS(U32 *pTargetCFIS);
void HAL_SataConstructAndSendPIOSetupFIS(const BOOL, const BOOL, const U8, const BOOL);
void HAL_SetSendSDBFISReady(U8 ucCmdTag);
void HAL_ClearSendSDBFISReady(U8 ucCmdTag);
void HAL_SataSignatureSendGoodStatus(void);
void HAL_SataFeedbacktoHW(U8 ucCmdTag, U8 ucCmdProtocol);
void HAL_SataClearPIOCmdPending(void);
#endif

/********************** FILE END ***************/

