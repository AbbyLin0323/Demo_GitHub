/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_SataIO.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    17:20:35
Description :
Others      :
Modify      :
****************************************************************************/

#ifndef _HAL_SATAIO_H
#define _HAL_SATAIO_H

#include "BaseDef.h"
#include "HAL_SataDefine.h"
#include "HAL_Define.h"
#include "HAL_SataDSG.h"
/**********************************************
For identify device 
**********************************************/
//#define   MAX_ACCESS_LBA     256*1024*1024 *2   //256GB
#ifdef L2_FORCE_VIRTUAL_STORAGE
#define   MAX_ACCESS_LBA    VIRTUAL_STORAGE_MAX_LBA    // 64MB RAM Disk
#else
#define   MAX_ACCESS_LBA    MaxLBAInDisk//L2_FORCE_VIRTUAL_STORAGE// CE_NUM*(BLK_PER_CE*PG_PER_BLK*SEC_PER_BUF - RSVD_BLK_PER_CE) 
#endif
//#define   SYSCFG_PAGE_SIZE_BITS    14
//#define   SYSCFG_PAGE_SIZE    (1<<SYSCFG_PAGE_SIZE_BITS)
//#define   SYSCFG_PAGE_SIZE_MSK  (SYSCFG_PAGE_SIZE -1)


/***********************************************
SDC/SACMDM/SDMAC Registers Address Definition
************************************************/
#define SATA_BASE_ADDRESS    REG_BASE_SDC

//SATA device controller related registers
#define SDC_BASE_ADDRESS (SATA_BASE_ADDRESS + 0x0)

//PHY control registers
#define SATA_PHYCONFIG_REGISTER_BASE (SDC_BASE_ADDRESS + 0x0)
#define rSDC_PHYTestModeCtrl1    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x00))
#define rSDC_PHYTestModeCtrl2    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x01))
#define rSDC_PHYTestModeCtrl3    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x02))
#define rSDC_PHYControl1    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x03))
#define rSDC_PHYControl2    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x04))
#define rSDC_PHYControl3    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x05))
#define rSDC_PHYControl4    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x06))
#define rSDC_PHYControl5    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x07))
#define rSDC_PHYControl6    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x08))
#define rSDC_PHYControl7    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0A))
#define rSDC_PHYControl8    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0B))
#define rSDC_PHYControl9    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0C))
#define rSDC_PHYControl10   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0D))
#define rSDC_PHYControl11   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0E))
#define rSDC_PHYControl12   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x0F)) //rSDC_SHR_LockControl
#define rSDC_PHYControl13   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x10))
#define rSDC_PHYControl14   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x11))
#define rSDC_PHYControl15   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x12))

#define rSDC_AFEControl3    (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x09))
#define rSDC_AFEControl54   (*(volatile U8 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x47))

#define rSDC_SendSDBFISReady   (*(volatile U32 *)(SATA_PHYCONFIG_REGISTER_BASE + 0x54))

//Link and command status control
#define rSDC_LinkStatus (*(volatile U16 *)(SDC_BASE_ADDRESS + 0x70))
#define DPM_PWR_STATE_NO_COMM       0x00
#define DPM_PWR_STATE_IF_IN_ACTIVE  0x01
#define DPM_PWR_STATE_IF_IN_PARTIAL 0x02
#define DPM_PWR_STATE_IF_IN_SLUMBER 0x06

#define rSDC_ErrorStatus (*(volatile U16 *)(SDC_BASE_ADDRESS + 0x74))
#define BIT_SDC_ERROR_STATUS_RXNRPE (1<<8) // Tx Data Fis
#define BIT_SDC_ERROR_STATUS_RXNRTE (1<<9) // Tx NonData Fis
#define BIT_SDC_ERROR_STATUS_RXNRDE (1<<12)// Rx Data Fis
#define BIT_SDC_ERROR_STATUS_RXRDIE (1<<0) // Rx NonData Fis

#define rSDC_LinkControl (*(volatile U16 *)(SDC_BASE_ADDRESS + 0x78))
#define rSDC_NCQActive (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x7C))
#define rSDC_NCQOutstd (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x80))

// Shadow register block firmware programming lock control
#define rSDC_SHR_LockControl (*(volatile U8 *)(SDC_BASE_ADDRESS + 0x0F))    //VT3492->VT3514: 0x31->0x0F
#define BIT_SDC_SHR_PROG_EN (1<<3)
#define BIT_FW_CMDINT_EN    (1<<1)  /* if en, HW will interrupt FW when NCQ cmd Data xfer finished */

//Shadow block registers
#define SATA_COMMAND_BLOCK_REGISTER_BASE (SDC_BASE_ADDRESS + 0x84)

#define rSDC_DATA    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 0))
#define rSDC_FEATURE_ERROR    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 1))
#define BIT_SDC_FEATURE_ERROR_MED   (1<<0)
#define BIT_SDC_FEATURE_ERROR_NM    (1<<1)
#define BIT_SDC_FEATURE_ERROR_ABRT  (1<<2)
#define BIT_SDC_FEATURE_ERROR_MCR   (1<<3)
#define BIT_SDC_FEATURE_ERROR_IDNF  (1<<4)
#define BIT_SDC_FEATURE_ERROR_MC    (1<<5)
#define BIT_SDC_FEATURE_ERROR_WP    (1<<6)
#define BIT_SDC_FEATURE_ERROR_ICRC  (1<<7)

#define rSDC_SECCNT    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 2))
#define rSDC_LBALOW    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 3))
#define rSDC_LBAMID    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 4))
#define rSDC_LBAHIGH    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 5))
#define rSDC_DEVICE_HEAD    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 6))
#define BIT_SDC_DEV_LBA_ENABLE (1 << 6)
#define rSDC_COMMAND_STATUS    (*(volatile U8 *)(SATA_COMMAND_BLOCK_REGISTER_BASE + 7))
#define BIT_SDC_COMMAND_STATUS_ERR  (1<<0)
#define BIT_SDC_COMMAND_STATUS_DRQ  (1<<3)
#define BIT_SDC_COMMAND_STATUS_CD   (1<<4)
#define BIT_SDC_COMMAND_STATUS_DF   (1<<5)
#define BIT_SDC_COMMAND_STATUS_DRDY (1<<6)
#define BIT_SDC_COMMAND_STATUS_BSY  (1<<7)

#define SATA_CONTROL_BLOCK_REGISTER_BASE (SDC_BASE_ADDRESS + 0x8C)
#define rSDC_CONTROL_ALTSTATUS (*(volatile U8 *)(SATA_CONTROL_BLOCK_REGISTER_BASE + 0))

#define SATA_EXP_COMMAND_BLOCK_REGISTER_BASE (SDC_BASE_ADDRESS + 0x90)
#define rSDC_EXP_FEATURE    (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 1))
#define rSDC_EXP_SECCNT    (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 2))
#define rSDC_EXP_LBALOW    (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 3))
#define rSDC_EXP_LBAMID    (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 4))
#define rSDC_EXP_LBAHIGH    (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 5))
#define rSDC_END_STATUS    (*(volatile U8 *)(SATA_EXP_COMMAND_BLOCK_REGISTER_BASE + 7))

//Parameters for D2H FIS construction
#define SATA_COMMAND_PARAMETER_REGISTER_BASE (SDC_BASE_ADDRESS + 0x98)
#define rSDC_PTDef    (*(volatile U8 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 0))
#define rSDC_PIOXferCountLow    (*(volatile U8*)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 1))
#define rSDC_PIOXferCountHigh    (*(volatile U8*)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 2))
#define rSDC_DMAXferCount    (*(volatile U32 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 3))
#define rSDC_DMABuffOffset        (*(volatile U32 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 7))
#define rSDC_DMABuffIDLow    (*(volatile U32 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 11))
#define rSDC_DMABuffIDHigh    (*(volatile U32 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 15))
//#define rSDC_FISDirInt    (*(volatile U32 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 19))
#define rSDC_FISDirInt    (*(volatile U8 *)(SATA_COMMAND_PARAMETER_REGISTER_BASE + 19)) //[20090701]JackeyChai: can not trigger a access cross DWORD boundary
#define BIT_SDC_FIS_DIRFLAG (1 << 0)
#define BIT_SDC_FIS_INTFLAG (1 << 1)

//Miscellaous SDC registers
#define rSDC_IOControl *(volatile U8 *)(SDC_BASE_ADDRESS + 0xAC)
#define BIT_SDC_IOCTRL_SENDBISTACT_FIS (1 << 0)
#define BIT_SDC_IOCTRL_SENDSDB_FIS (1 << 1)
#define BIT_SDC_IOCTRL_SENDDMADATA_FIS (1 << 2)
#define BIT_SDC_IOCTRL_SENDPIODATA_FIS (1 << 3)
#define BIT_SDC_IOCTRL_SENDDMASETUP_FIS (1 << 4)
#define BIT_SDC_IOCTRL_SENDDMAACT_FIS (1 << 5)
#define BIT_SDC_IOCTRL_SENDPIOSETUP_FIS (1 << 6)
#define BIT_SDC_IOCTRL_SENDREGD2H_FIS (1 << 7)

#define rSDC_IntSrcPending *(volatile U32 *)(SDC_BASE_ADDRESS + 0xB0)
#define BIT_SDC_INTSRC_EOTPENDING (1<<0)
#define BIT_SDC_INTSRC_LPM (1<<1)
#define BIT_SDC_INTSRC_FIFOFULL (1<<2)
#define BIT_SDC_INTSRC_SERROR (1<<3)
#define BIT_SDC_INTSRC_SOFTRESET (1 << 4)
#define BIT_SDC_INTSRC_TXOP (1 << 7)
#define BIT_SDC_INTSRC_NCQFINISH   (1<<9)
#define BIT_SDC_INTSRC_FIS_CONTROL (1 << 11)
#define BIT_SDC_INTSRC_FIS_COMMAND (1 << 12)
#define BIT_SDC_INTSRC_OOB_DONE (1 << 13)
#define BIT_SDC_INTSRC_RXPIO_DATA (1 << 14)
#define BIT_SDC_INTSRC_RXDMACMPL (1<<15)
#define BIT_SDC_INTSRC_COMRESET_RCV    (1<<16)
#define BIT_SDC_INTSRC_PENDING     (1<<17)
#define BIT_SDC_INTSRC_SYNC_ESCAPE (1<<18)

#define rSDC_NCQCMD_CLR_TAG     *(volatile U32 *)(SDC_BASE_ADDRESS + 0xB4)
#define rSDC_NCQCMD_HOLD_TAG    *(volatile U32 *)(SDC_BASE_ADDRESS + 0xBC)

#define rSDC_IntMask *(volatile U16 *)(SDC_BASE_ADDRESS + 0xB8)
#define rSDC_ErrorMask *(volatile U8 *)(SDC_BASE_ADDRESS + 0xBA)

#define rSDC_FISDelayControl    (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xC0))
#define TXSETDEVCOMPL_DLY_EN    (1<<31)

#define rSDC_ControlRegister    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xCC))
#define FW_DECODE_ENABLE    (1<<7)
#define FW_DMAEXE_STE_EN    (1<<6)
#define PARTSLU_EN          (1<<5)
#define HW_COMWAKE_EN       (1<<4)
#define HW_SLUMBER_EN       (1<<3)
#define HW_PARTIAL_EN       (1<<2)
#define DIPM_EN             (1<<1)
#define HW_PWR_EN           (1<<0)

#define rSDC_FW_DECODE          (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xCD))
#define rSDC_FW_Ctrl     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xCE))
#define FW_CLR_NCQEXE       (1)
#define FW_CFGCMD_DONE      (1<<1)  //big busy
#define HD_Err_clr          (1<<2)
#define CLR_PIOCMD_DATA     (1<<3)  //small busy

#define rSDC_SHRLCH_LBA28       (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xD0)) //34
#define rSDC_SHRLCH_LBA48       (*(volatile U32 *)(SDC_BASE_ADDRESS + 0xD4))
#define rSDC_SHRLCH_SECCNT8     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD8))
#define rSDC_SHRLCH_SECCNT16     (*(volatile U16 *)(SDC_BASE_ADDRESS + 0xD8))
#define rSDC_SHRLCH_FEATURE8    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDA))
#define rSDC_SHRLCH_FEATURE16  (*(volatile U16 *)(SDC_BASE_ADDRESS + 0xDA))
#define rSDC_SHRLCH_COMMAND     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDC))
#define rSDC_SHRLCH_NCQTAG       (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDE))
#define rSDC_SHRLCH_EN             (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE0))
#define rSDC_SHRLCH_CONTROL     (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE1))
#define rSDC_SHRLCH_DEV6           (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE2))
#define rSDC_SHRLCH_AUXILIARY   (*(volatile U32 *)(SDC_BASE_ADDRESS + 0x40))    /* VT3514 add to support sata 3.2 */
#define rSDC_SHRLCH_ICC          (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE7))    /* VT3514 add to support sata 3.2 */

#define rSDC_LATH_FEATURE_ERROR    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDA))
#define rSDC_LATH_SECCNT    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD8))
#define rSDC_LATH_LBALOW    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD0))
#define rSDC_LATH_LBAMID    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD1))
#define rSDC_LATH_LBAHIGH    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD2))
#define rSDC_LATH_DEVICE_HEAD    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDD))
#define rSDC_LATH_COMMAND_STATUS    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDC))
#define rSDC_LATH_CONTROL_ALTSTATUS (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE1))
#define rSDC_LATH_EXP_FEATURE    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xDB))
#define rSDC_LATH_EXP_SECCNT    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xD9))
#define rSDC_LATH_EXP_LBALOW    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE4))
#define rSDC_LATH_EXP_LBAMID    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE5))
#define rSDC_LATH_EXP_LBAHIGH    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE6))
#define rSDC_LATH_EXP_STATUS    (*(volatile U8 *)(SDC_BASE_ADDRESS + 0xE3))


//SATA command management controller related registers
#define SACMDM_BASE_ADDRESS (SATA_BASE_ADDRESS + 0x100)

/* first DSG id register, every command tag owns one U8 register
    Usage: rFIRST_DSG_ID[tag] = dsg id
*/
#define rFIRST_DSG_ID           ((volatile U8 *)SACMDM_BASE_ADDRESS)

/*    First RPRD index for each command slot:
Use as an array:
rSACMDM_FRPRDIndex[n] = ucCurrPRDIndex;
*/
#define rSACMDM_SlotEntryRPRDIndex    ((volatile U8 *)(SACMDM_BASE_ADDRESS + 0x20))

/*    Next RPRD index for each RPRD:
Use as an array:
rSACMDM_RPRDNextIndex[n] = i++;
*/
#define rSACMDM_RPRDNextIndex    ((volatile U8 *)(SACMDM_BASE_ADDRESS + 0x40))


/*    Start LBA for each command slot (for TCG encryption):
Use as an array:
rSACMDM_SlotLBATable[n] = ulCurrLBA;
*/
#define rSACMDM_SlotLBATable    ((volatile U32 *)(SACMDM_BASE_ADDRESS + 0x80))

#define rSACMDM_CACHE_STATUS_BASE_ADDRESS_SRAM (*(volatile U32 *)(SACMDM_BASE_ADDRESS + 0x08c))


//SATA main storage DMA controller related registers
#define SDMAC_BASE_ADDRESS (SATA_BASE_ADDRESS + 0x300)

#define rSDMAC_DataBuffBase     (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x0))
//BUFFSIZE[2:0] 0:4K, 1:8K, 2:16K, 3:32K, 4:64K, 5:128K 
#define BUFFSIZE_4K     0
#define BUFFSIZE_8K     1
#define BUFFSIZE_16K    2
#define BUFFSIZE_32K    3
#define BUFFSIZE_64K    4
#define BUFFSIZE_128K   5

#define DUMMYDATA   (1<<3)
#define SCRUMBLE    (1<<4)
#define SDBFISwDMA  (1<<7)
#define GLOBALERROR (1<<8)
#define HOLDXFER    (1<<9)  /* PRD hold DMA */
#define BYPASSEN    (1<<10)


#define rSDMAC_ScrambleKey      (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x4))
#define rSDMAC_PRDAttribute     (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x10))
#define rSDMAC_PRDMemAddr       (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x14))
#define rSDMAC_PRDXferLen       (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x18))
#define rSDMAC_PRDBuffMapID     (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x1C))
#define rSDMAC_PRDLBALow        (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x20))
#define rSDMAC_PRDLBAHigh       (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x24))
#define rSDMAC_Status           (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x30))
#define rSDMAC_CurBufMapId      (*(volatile U8 *)(SDMAC_BASE_ADDRESS + 0x34))

#define rSDMAC_ErrorHandling    (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x40))
#define rSDMAC_CmdType    (*(volatile U32 *)(SDMAC_BASE_ADDRESS + 0x44))
#define rTOP_REGGLB40 (*(volatile U32*)(REG_BASE_GLB+0x40))
#define rTOP_REGGLB68 (*(volatile U32*)(REG_BASE_GLB+0x68)) // PER related
#define RIIC_EN     (1<<9)
#define RGPIO_EN    (1<<10)
#define RSPI_EN     (1<<11)
#define RUART_EN    (1<<12)
#define RJTAG_EN    (1<<13)

//PIO protocol handler state
#define SATA_PIO_NOCMD 0x00
#define SATA_PIO_NEWCMD 0x01
#define SATA_PIO_SETUP 0x02
#define SATA_PIO_DATA_IN 0x03
#define SATA_PIO_DATA_OUT 0x04
#define SATA_PIO_FINISH 0x05
#define SATA_PIO_BUILD_READ_PRD 0x06

#define PROTOCAL_SELECT(Protocol)   ((HCMD_PROTOCOL_PIO == (Protocol))?PROT_PIO:PROT_DMA_FPDMA)

/* Enable/Disable Firmware to Configure ERROR and STATUS in Shadow Register */
#define Lock_ShadowRegister()   (rSDC_SHR_LockControl |= BIT_SDC_SHR_PROG_EN)
#define UnLock_ShadowRegister() (rSDC_SHR_LockControl &= ~BIT_SDC_SHR_PROG_EN)


typedef struct _SDC_PRD_CONTROL
{
    U32 WP_RPTR:6;//Write PRD address gen
    U32 Rsvd0:2;

    U32 CURRPRD:6;// Read PRD address gen
    U32 CUR_FULL0:1;
    U32 CUR_FULL:1;

    U32 WP_WPTR:6;
    U32 Rsvd1:1;
    U32 WP_FULL:1;

    U32 PRD_DBG:8;
}SDC_PRD_CONTROL;

/* PRD structures */
typedef struct _ATAPROTINFO {
    U32 usCmdXferLenSect: 16;
    U32 ucBuffMapID: 6;
    U32 bFirstPRDFlag: 1;
    U32 bXferDirct: 1;// 1 = write command
    U32 ucCmdTag: 5;
    U32 bDMASetupInt: 1;
    U32 uResv1: 1;
    U32 bHWProtlEn: 1;// 0 = for PIO command
} ATAPROTINFO, *PATAPROTINFO;

typedef struct _SDMAXFERINFO {
    U32 ucPRDXferLenSect: 8;
    U32 ucBuffAddrHSect: 8;
    U32 uResv2: 5;
    U32 bCachStaLocSel: 1;// 1 = SRAM
    U32 bUpdWrBuffMapEn: 1;
    U32 bBuffLocSel: 1;// 1 = SRAM(OTFB)
    U32 bXferEndIntEn: 1;
    U32 bAutoAckHostEn: 1;
    U32 ucEcpKeySel: 2;
    U32 bEcpEn: 1;
    U32 bStuffDataEn: 1;
    U32 uResv3: 1;
    U32 bXferEndFlg: 1;
} SDMAXFERINFO, *PSDMAXFERINFO;

typedef struct _SGLENTRY {
    U32 ucBuffMapStartSect: 8;
    U32 ucXferLenSect: 8;
    U32 usBuffAddrLSect: 16;
} SGLENTRY, *PSGLENTRY;

typedef struct _CACHSTAINFO {
    U32 usUpdAddr: 16;
    U32 ucUpdValue: 8;
    U32 bUpdEn: 1;
    U32 uResv: 7;
} CACHSTAINFO, *PCACHSTAINFO;

typedef struct _SATA_PRD_ENTRY_ {
    /* DWORD 0 */
    ATAPROTINFO ATAInfo;

    /* DWORD 1 */
    SDMAXFERINFO XferInfo;

    /* DWORD 2 - DWORD 5 */
    union{
        SGLENTRY DMAPRD[4];
        U32 DebugInfo[4];// NOTE: can not overwrite valid DMAPRD
    };

    /* DWORD 6 - DWORD 7 */
    CACHSTAINFO CchStsInfo[2];
} SATA_PRD_ENTRY;

#define PRD_SIZE_DW 8
#define DSG_SIZE_DW 8

typedef enum _SATA_PRD_SPLIT_SIZE_
{
    PRD_SPLIT_SIZE_4K = 0,
    PRD_SPLIT_SIZE_8K,
    PRD_SPLIT_SIZE_16K,
    PRD_SPLIT_SIZE_32K,
    PRD_SPLIT_SIZE_64K,
    PRD_SPLIT_SIZE_128K
}SATA_PRD_SPLIT_SIZE;

#if(SEC_PER_BUF_BITS == 6) //32K Buffer
#define CURRENT_PRD_SPLIT_SIZE PRD_SPLIT_SIZE_32K
#else   //16K Buffer
#define CURRENT_PRD_SPLIT_SIZE PRD_SPLIT_SIZE_16K
#endif

#define RPRD_NUM    64
#define WPRD_NUM    64

/**/
typedef struct _HCMD
{
    U32 ulCmdLba;       //LBA

    U8  ucCmdCode;    
    U8  ucCmdTag;        //command tag for NCQ commands
    U8  ucCmdProtocol;  //NCQ/DMA/NCQ
    U8  ucCmdRW;

    union
    {
        U32 ulCmdSectorCnt;
        U32 ulSHRFeature16;
    };
    union
    {
        U32 ulCmdRemSector;    //remain length for command split, initial to ulSectorCount
        U32 ulSHRCount16;
    };
    
    U8  ucCmdType;        //Data/NoneData
    U8  ucNCQSubCmd;
    U8  ucSubCmdSpec;    
    U8  ucCmdStatus;
}HCMD;

typedef enum
{
    HCMD_STATE_NONE = 0,
    HCMD_STATE_RECEIVED,
    HCMD_STATE_PROCESSING,
    HCMD_STATE_FW_DONE,
    HCMD_STATE_SATA_DONE
}HCMD_STATE;

/*
The LPNOffsetOUT need be config during full hit read and write to cache.It is the postition informaiton of 
request date in cache,we need config sata with it.

*/
typedef struct _SUBCMD_ADD_INFO
{
    /* total 4 DWORD */
    /*Input SUBCMD info*/
    U32 ulSubCmdLBA;

    U8 ucSubCmdOffsetIN;
    U8 ucSubCmdlengthIN;
    U8 ucSubLPNOffsetIN;
    U8 ucSubLPNCountIN;

    U16 usSubCmdCacheTAG;
    U8  ucFullHitCache;  //If hit, set to '1'; Hit can be either read hit cache or write hit cache
    U8  ucPuNum;

    /*Output Buffer info*/
    U8 ucBufLPNOffsetOUT;
    U8 ucBufLPNCountOUT;
    U8 ucSubCmdOffsetOUT;
    U8 ucSubCmdlengthOUT;

}SUBCMD_ADD_INFO;


typedef struct _SUBCMD
{
    /* 8 DWORD */
    HCMD *pHCMD;
    
    SUBCMD_ADD_INFO SubCmdAddInfo;   /*4 DWORD */

    /* union is the temporary, it(PRD) would be deleted finally */
    union
    {
        U8 SubDSGId;
        U8 SubPRDId;
    };
    U8 SubNextDsgId;
    U8 SubCmdId;
    U8 SubCmdStage;
    U8 SubCmdHitResult;

    U8 SubCmdFirst;
    U8 SubCmdLast;
    U16 SubCmdPhyBufferID;

    //20130606: only cache status 0 used
    U32 CacheStatusAddr;

  /* 1 or 2 DWORD */
  U8 LpnSectorBitmap[LPN_PER_BUF];
    
}SUBCMD;

#define SUBCMD_SIZE   (sizeof(SUBCMD)/4)


typedef enum _HCMD_PROTOCOL_
{
    HCMD_PROTOCOL_PIO = 4,  //to match VT3514 hw design, start from 4.
    HCMD_PROTOCOL_DMA = 5,
    HCMD_PROTOCOL_NCQ = 6,
    HCMD_PROTOCOL_RSV = 7
}HCMD_PROTOCOL;

typedef enum _HCMD_TYPE
{
    HCMD_TYPE_DATA = 0,
    HCMD_TYPE_NONDATA
}HCMD_TYPE;

extern void HAL_SataGetPRDControl(void);
extern U8 HAL_SataBuildReadPRD(SUBCMD* pSubCmd);
extern U8 HAL_SataBuildWritePRD(SUBCMD* pSubCmd);
//void HAL_SataInitialize(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
void HAL_ResetSDC(void);
void HAL_SataInitialize(void);

BOOL HAL_SataIsFISXferAvailable(void);
void HAL_SataSendSuccessStatus(void);
void HAL_SataSendAbortStatus(void);
void HAL_SataSendSetDevBitFIS(void);
void HAL_SataSendRegD2HFIS(void);
void HAL_SataSendPIOSetupFIS(void);
void HAL_SataSendPIODataFIS(void);
U8 HAL_SataGetCmdFromSRB(void);
void HAL_SataSendGoodStatus(void);


U8    HAL_SataGetNextReadPRD(void);
U8 HAL_SataGetNextWritePRD(void);
void HAL_SataTriggerReadPRD(void);

void HAL_SataConstructAndSendPIOSetupFIS(const BOOL, const BOOL, const U8, const U8);

extern HCMD HostCmdSlot[NCQ_DEPTH];
extern U32 g_ulDramFreeAddr;

#define FIS_Delay

extern U8   HAL_SataBuildReadDSG(SUBCMD* pSubCmd);
extern U8   HAL_SataBuildWriteDSG(SUBCMD* pSubCmd);

BOOL HAL_SataBuildSpecilReadDSG(HCMD* pCurHCMD, U32 ulDramAddr);
BOOL HAL_SataBuildSpecilWriteDSG(HCMD* pCurHCMD, U32 ulDramAddr);
void HAL_SetSendSDBFISReady(U8 ucCmdTag);
void HAL_ClearSendSDBFISReady(U8 ucCmdTag);

#define HAL_HCmdNone(tag)       (HostCmdSlot[(tag)].ucCmdStatus = HCMD_STATE_NONE)
#define HAL_HCmdReceived(tag)   (HostCmdSlot[(tag)].ucCmdStatus = HCMD_STATE_RECEIVED)
#define HAL_HCmdProcessing(tag) (HostCmdSlot[(tag)].ucCmdStatus = HCMD_STATE_PROCESSING)
#define HAL_HCmdFWDone(tag)     (HostCmdSlot[(tag)].ucCmdStatus = HCMD_STATE_FW_DONE)
#define HAL_HCmdSataDone(tag)   (HostCmdSlot[(tag)].ucCmdStatus = HCMD_STATE_SATA_DONE)

void cosim_SetCacheStsAddr(U8 ucCmdTag, U32 ulCSAddress);
void cosim_SetCacheStatus(U32 ulCSAddress);
#endif

/********************** FILE END ***************/

