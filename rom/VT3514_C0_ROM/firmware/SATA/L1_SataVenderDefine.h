/*************************************************
Copyright (C), 2009, VIA Tech. Co., Ltd.
Filename: L1_SataVenderDefine.h                                            
Version: 0.1                                                 
Date: 2009-08-24                                            
Author: JackeyChai

Description: ATA vender define commands codes 
Others:

Modification History:
JackeyChai,       2009-08-24,      first created
Blakezhang      2013-03-04,      blakezhang porting to C0 FW
*************************************************/
#ifndef __SATA_VEN_CMD_H__
#define __SATA_VEN_CMD_H__

#define VD_FEA_GET_DRAM_SIZE    0x00    /*get DRAM size in byte unit*/

#define VD_FEA_READ_DRAM_DMA    0x01    /*read DRAM data in DMA mode*/
#define VD_READ_SEQ_FIRST       1
#define VD_READ_SEQ_LAST        2

#define VD_FEA_DISK_STATUS_CHK 0x2

/*Begin: add by henryluo 2010-12-27 for MPT tool */
#define VD_READ_REG     0xC0
#define VD_WRITE_REG    0xC1
#define VD_WISHBONE_READ    0xC2
#define VD_WISHBONE_WRITE   0xC3
#define VD_FIRMWARE_RUN     0xC4
#define VD_READ_STATUS     0xC5

#define VD_NFC_IDCODE       0xD0
#define VD_NFC_INIT       0xD1
#define VD_NFC_WRITE       0xD2
#define VD_NFC_READ       0xD3
#define VD_NFC_ERASE       0xD4
#define VD_NFC_RST_PU       0xD5
#define VD_LLF_BUILD_SYSCFG       0xE0
#define VD_LLF_INIT      0xE1
#define VD_LLF_BT_INIT       0xE2
#define VD_LLF_BBT_READ       0xE3
#define VD_LLF_PRE_SAVE_BLK      0xE4
#define VD_LLF_SAVE_ROM_TABLE       0xE5
#define VD_LLF_SAVE_LOADER       0xE6
#define VD_LLF_PRE_SAVE_FIRMWARE       0xE7
#define VD_LLF_PRE_SAVE_SATA_PARAMETER       0xE8
#define VD_LLF_SAVE_TABLE       0xE9
#define VD_SPI_ROM_BURNNING       0xEA
/*End: add by henryluo 2010-12-27 for MPT tool */

/*Begin: add by henryluo 2010-12-01 for system monitor tool */
#define VD_READ_GLOBAL_STATISTIC_INFO    0xF0    /* get device global statistic info */
#define VD_READ_PLANE_STATISTIC_INFO    0xF1    /* get device plane statistic info */
#define VD_READ_BBT    0xF2    /* get device bad block table */
#define VD_READ_PBIT    0xF3    /* get device pbit */
#define VD_READ_PLANE_DPTR    0xF4    /* get device plane dptr */
#define VD_READ_SYSCFG    0xF5    /* get device system config */
/*End: add by henryluo 2010-12-01 for system monitor tool */

/*Begin: add by henryluo 2010-12-17 for enhancement tool */
#define VD_FW_UPDATE_PREP    0xF6    /* prepare for firmware update */
#define VD_FW_UPDATE_SAVE    0xF7    /* save firmware to back area */
#define VD_FW_UPDATE_EXEC    0xF8    /* copy firmware to pu0 block1 */
#define VD_PROVISON_ADJUST    0xF9    /* adjust rsvd block pre pu */
/*End: add by henryluo 2010-12-17 for enhancement tool */

#define FIRMWARE_SEC_PER_PG     8
#define FIRMWARE_PG_SZ_BITS     12
#define FIRMWARE_BLK_SZ_BITS     (8 + FIRMWARE_PG_SZ_BITS)

typedef void (*func_t)(void);

typedef struct _IDEREGSx
{
    U8  bFeaturesReg;
    U8  bSectorCountReg;
    U8  bLBALowReg;//bSectorNumberReg; /*LBA low*/
    U8  bLBAMidReg;//bCylLowReg;       /*LBA Mid*/      
    U8  bLBAHighReg;//bCylHighReg;      /*LBA Hight*/  
    U8  bDriveHeadReg;
    U8  bCommandReg;
    U8  bReserved;
} IDEREGSx, *PIDEREGSx, *LPIDEREGSx;

extern BOOL DRAM_ATTR L1_SataCmdVenderDefine(void);

#endif
