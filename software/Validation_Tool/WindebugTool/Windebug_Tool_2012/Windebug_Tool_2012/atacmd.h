/*************************************************
Copyright (C), 2009, VIA Tech. Co., Ltd.
Filename: satacmd.h                                            
Version: 0.1                                                 
Date: 2009-08-21                                            
Author: JackeyChai

Description: ATA commands codes define
Others:

Modification History:
JackeyChai,       2009-08-21,      first created
*************************************************/
#ifndef __SATA_CMD_H__
#define __SATA_CMD_H__

/*Control Command*/
#define ATA_CMD_EXECUTE_DEVICE_DIAGNOSTIC    0x90

#define ATA_CMD_FLUSH_CACHE				     0xE7
#define ATA_CMD_FLUSH_CACHE_EXT              0xEA

#define ATA_CMD_IDENTIFY_DEVICE			     0xEC
#define ATA_CMD_SET_FEATURES				 0xEF
#define ATA_CMD_SET_MULTIPLE_MODE		     0xC6

#define ATA_CMD_READ_LOG_EXT				 0x2F
#define ATA_CMD_WRITE_LOG_EXT				 0x3F
#define ATA_LOG_DIRECTORY					 0x00
#define ATA_LOG_NCQ_ERROR					 0x10
#define ATA_LOG_SMART_ERR					0x01
#define ATA_LOG_COMPREHENSIVE_SMART_ERR	0x02
#define ATA_LOG_EXTEND_COMPREHENSIVE_SMART_ERR	0x03
#define ATA_LOG_SMART_SELF_TEST	0x06
#define ATA_LOG_EXTEND_SMART_SELF_TEST	0x07
#define ATA_LOG_SELECTIVE_SELF_TEST	0x09
#define ATA_LOG_DELAYED_SECTOR	0x23

#define ATA_CMD_NOP						     0x00

/*PowerManagment */
#define ATA_CMD_CHECK_POWER_MODE             0xE5
#define ATA_CMD_IDLE						 0xE3
#define ATA_CMD_IDLE_IMMEDIATE				 0xE1
#define ATA_CMD_SLEEP						 0xE6
#define ATA_CMD_STANDBY					     0xE2
#define ATA_CMD_STANDBY_IMMEDIATE			 0xE0

/*Data Command*/
/*PIO Data command*/
#define ATA_CMD_READ_SECTOR				     0x20
#define ATA_CMD_READ_SECTOR_EXT			     0x24

#define ATA_CMD_WRITE_SECTOR				 0x30
#define ATA_CMD_WRITE_SECTOR_EXT			 0x34

#define ATA_CMD_READ_MULTIPLE				 0xC4
#define ATA_CMD_READ_MULTIPLE_EXT			 0x29

#define ATA_CMD_WRITE_MULTIPLE				 0xC5
#define ATA_CMD_WRITE_MULTIPLE_EXT			 0x39

/*DMA Data command*/
#define ATA_CMD_READ_DMA					 0xC8
#define ATA_CMD_READ_DMA_EXT				 0x25

#define ATA_CMD_WRITE_DMA					 0xCA
#define ATA_CMD_WRITE_DMA_EXT				 0x35
#define ATA_CMD_DATA_SET_MANAGEMENT	0x06

/*NCQ Data command*/
#define ATA_CMD_READ_FPDMA_QUEUED			 0x60
#define ATA_CMD_WRITE_FPDMA_QUEUED			 0x61

/*Other*/
#define ATA_CMD_READ_VERIFY_SECTOR			 0x40
#define ATA_CMD_READ_VERIFY_SECTOR_EXT		 0x42

/*Option Command*/
/*HPA Command*/
#define ATA_CMD_READ_NATIVE_MAX_ADDRESS	     0xF8
#define ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT  0x27
#define ATA_CMD_SET_MAX_ADDRESS			     0xF9
#define ATA_CMD_SET_MAX_ADDRESS_EXT          0x37

/*Security Command*/
#define ATA_CMD_SECURITY_DISABLE_PASSWORD    0xF6
#define ATA_CMD_SECURITY_ERASE_PREPARE       0xF3
#define ATA_CMD_SECURITY_ERASE_UNIT          0xF4
#define ATA_CMD_SECURITY_FREEZE_LOCK         0xF5
#define ATA_CMD_SECURITY_SET_PASSWORD        0xF1
#define ATA_CMD_SECURITY_UNLOCK              0xF2

/*TRUST command: TCG and SILO*/
#define ATA_CMD_TRUST_NON_DATA      0x5B
#define ATA_CMD_TRUST_SEND                 0x5E
#define ATA_CMD_TRUST_SEND_DMA       0x5F
#define ATA_CMD_TRUST_RECV                  0x5C
#define ATA_CMD_TRUST_RECV_DMA        0x5D

/*other*/
#define ATA_CMD_DOWNLOAD_MICROCODE           0x92
#define ATA_CMD_RECALIBRATE		0x10

/*old command*/
#define ATA_CMD_INITIALIZE_DEVICE_PARAMETERS 0x91

/*SMART command*/
#define ATA_CMD_SMART 0xB0
#define SMART_FEATURE_READ_DATA 0xD0 /*SMART read data*/
#define SMART_FEATURE_READ_THRESHOLD 0xD1 /*SMART read data*/
#define SMART_FEATURE_AUTOSAVE_DISEN 0xD2 /*SMART AutoSave Disable/Enable*/
#define SMART_EXEC_OFFLINE_IMMEDIATE 0xD4
#define SMART_READ_LOG   0xD5
#define SMART_WRITE_LOG 0xD6
#define SMART_ENABLE_OPERATIONS 0xD8
#define SMART_DISABLE_OPERATIONS 0xD9
#define SMART_RETURN_STATUS 0xDA

#define SMART_RCV_DRIVE_DATA 0x0007c088
#define SMART_GET_VERSION    0x00074080

/*vender define commands: for factory untility U32erface, detailes define*/
#define	ATA_CMD_VENDER_DEFINE		0xFF
    #define SUBCMD_VIA_READ_REG 0xC0
    #define SUBCMD_VIA_WRITE_REG 0xC1
    #define SUBCMD_VIA_MEM_READ 0xC2
    #define SUBCMD_VIA_MEM_WRITE 0xC3
    #define SUBCMD_VIA_EXEC_FROM_ADDR 0xC4
    #define SUBCMD_VIA_SWITCH_BUFFER 0xC5

//SetFeature Macro
#define ENABLE_WRITE_CACHE   0x02
#define DISABLE_WRITE_CACHE  0x82

#define SET_TRANSFER_MODE    0x03

#define ENABLE_SATA_FEATURE  0x10
#define DISABLE_SATA_FEATURE 0x90

#define  SATA_FEATURE_NONZERO_OFFSET			0x1
#define  SATA_FEATURE_AUTO_ACTIVATE				0x2
#define  SATA_FEATURE_GUARANTEED_INORDER        0x4  
#define  SATA_FEATURE_SETTING_PRESERVATION		0x6

#define SATA_POWER_IDLE           0x80 
#define SATA_POWER_STANDBY        0x00
#define SATA_POWER_ACTIVEORIDLE   0xff
#define SATA_POWER_SLEEP          0x20     /*can not find in PATA Spec*/

#endif


