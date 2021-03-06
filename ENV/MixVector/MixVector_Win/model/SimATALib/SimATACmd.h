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

Filename     :   SataCommand.h                                         
Version      :   0.1                                              
Date         :   2008.8.11                                         
Author       :   PeterXiu

Description:  the macro definition of SATA/PATA COMMAND
Others: 
Modification History:
20080811 create

*******************************************************************/
#ifndef _ATA_CMD_H
#define _ATA_CMD_H

//Control Command
#define ATA_CMD_EXECUTE_DEVICE_DIAGNOSTIC    0x90

#define ATA_CMD_FLUSH_CACHE                     0xE7
#define ATA_CMD_FLUSH_CACHE_EXT              0xEA

#define ATA_CMD_IDENTIFY_DEVICE                 0xEC
#define ATA_CMD_SET_FEATURES                 0xEF
#define ATA_CMD_SET_MULTIPLE_MODE             0xC6

#define ATA_CMD_READ_LOG_EXT                 0x2F
#define ATA_CMD_WRITE_LOG_EXT                 0x3F
#define LOG_PAGE_DIRECTORY                     0x00
#define LOG_PAGE_NCQ_ERROR                     0x10

#define ATA_CMD_NOP                             0x00
#define ATA_CMD_SEEK                            0x70


//PowerManagment 
#define ATA_CMD_CHECK_POWER_MODE             0xE5
#define ATA_CMD_IDLE                         0xE3
#define ATA_CMD_IDLE_IMMEDIATE                 0xE1
#define ATA_CMD_SLEEP                         0xE6
#define ATA_CMD_STANDBY                         0xE2
#define ATA_CMD_STANDBY_IMMEDIATE             0xE0

//Data Command
//PIO Data command
#define ATA_CMD_READ_SECTOR                     0x20
#define ATA_CMD_READ_SECTOR_EXT                 0x24

#define ATA_CMD_WRITE_SECTOR                 0x30
#define ATA_CMD_WRITE_SECTOR_EXT             0x34

#define ATA_CMD_READ_MULTIPLE                 0xC4
#define ATA_CMD_READ_MULTIPLE_EXT             0x29

#define ATA_CMD_WRITE_MULTIPLE                 0xC5
#define ATA_CMD_WRITE_MULTIPLE_EXT             0x39

//DMA Data command
#define ATA_CMD_READ_DMA                     0xC8
#define ATA_CMD_READ_DMA_EXT                 0x25

#define ATA_CMD_WRITE_DMA                     0xCA
#define ATA_CMD_WRITE_DMA_EXT                 0x35
#define ATA_CMD_DATA_SET_MANAGEMENT           0x06

//NCQ Data command
#define ATA_CMD_READ_FPDMA_QUEUED             0x60
#define ATA_CMD_WRITE_FPDMA_QUEUED             0x61

//NCQ NON-DATA command
#define ATA_CMD_NCQ_NON_DATA                0x63
#define ATA_CMD_SEND_FPDMA_QUEUED           0x64
#define ATA_CMD_RECEIVE_FPDMA_QUEUED        0x65

//Other
#define ATA_CMD_READ_VERIFY_SECTOR             0x40
#define ATA_CMD_READ_VERIFY_SECTOR_EXT         0x42
#define ATA_CMD_IDENTIFY_PACKET_DEVICE         0xA1
#define ATA_CMD_READ_BUFFER                    0xE4
#define ATA_CMD_WRITE_BUFFER                   0xE8

//Option Command
//HPA Command
#define ATA_CMD_READ_NATIVE_MAX_ADDRESS         0xF8
#define ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT  0x27
#define ATA_CMD_SET_MAX_ADDRESS                 0xF9
#define ATA_CMD_SET_MAX_ADDRESS_EXT          0x37

//Secure Command
#define ATA_CMD_SECURITY_DISABLE_PASSWORD    0xF6
#define ATA_CMD_SECURITY_ERASE_PREPARE       0xF3
#define ATA_CMD_SECURITY_ERASE_UNIT          0xF4
#define ATA_CMD_SECURITY_FREEZE_LOCK         0xF5
#define ATA_CMD_SECURITY_SET_PASSWORD        0xF1
#define ATA_CMD_SECURITY_UNLOCK              0xF2

//other
#define ATA_CMD_DOWNLOAD_MICROCODE           0x92
#define ATA_CMD_RECALIBRATE        0x10

//old command
#define ATA_CMD_INITIALIZE_DEVICE_PARAMETERS 0x91

//SMART command
#define ATA_CMD_SMART 0xB0
#define SMART_FEATURE_READ_DATA 0xD0 /*SMART read data*/
#define SMART_READ_ATTRIBUTE_THRESHOLDS 0xD1 /*SMART read thersholds*/
#define SMART_FEATURE_AUTOSAVE_DISEN 0xD2 /*SMART AutoSave Disable/Enable*/
#define SMART_EXEC_OFFLINE_IMMEDIATE 0xD4
#define SMART_READ_LOG   0xD5
#define SMART_CMD_WRITE_LOG 0xD6
#define SMART_ENABLE_OPERATIONS 0xD8
#define SMART_DISABLE_OPERATIONS 0xD9
#define SMART_RETURN_STATUS 0xDA

//vender define commands: for factory untility interface
#define    ATA_CMD_VENDER_DEFINE        0xFF
    #define  VIA_SUBCMD_MEM_READ                1
    #define  VIA_SUBCMD_MEM_WRITE               2
    #define  VIA_SUBCMD_FLASH_READ              3
    #define  VIA_SUBCMD_FLASH_WRITE             4
    #define  VIA_SUBCMD_FLASH_ERASE             5
    #define  VIA_SUBCMD_VAR_TABLE_ADDR          8
    #define  VIA_SUBCMD_TRACELOG_CONTROL        14

//SetFeature Macro
#define ENABLE_WRITE_CACHE   0x02
#define DISABLE_WRITE_CACHE  0x82

//SetFeature Macro
#define ENABLE_READ_LOOK_AHEAD   0xAA
#define DISABLE_READ_LOOK_AHEAD  0x55

#define ENABLE_POWER_ON_DEFAULTS  0xCC
#define DISABLE_POWER_ON_DEFAULTS 0x66

#define SET_TRANSFER_MODE    0x03

#define ENABLE_APM            0x05
#define DISABLE_APM            0x85

#define ENABLE_SATA_FEATURE  0x10
#define DISABLE_SATA_FEATURE 0x90

#define  SATA_FEATURE_NONZERO_OFFSET            0x1
#define  SATA_FEATURE_AUTO_ACTIVATE                0x2
#define  SATA_FEATURE_DIPM                      0x3
#define  SATA_FEATURE_GUARANTEED_INORDER        0x4  
#define  SATA_FEATURE_SETTING_PRESERVATION        0x6
#define SATA_FEATURE_DIAPTS 0x7
#define SATA_FEATURE_DEVSLP 0x9

#define SATA_POWER_ACTIVE          0x40
#define SATA_POWER_IDLE           0x80 
#define SATA_POWER_STANDBY        0x00
#define SATA_POWER_ACTIVEORIDLE   0xff
#define SATA_POWER_SLEEP          0x20     //can not find in PATA Spec

//Subcommand code and macros defined for vendor defined command
#define VIA_TRACE_GETLOGINFO 0xC0
#define VIA_TRACE_GETLOGDATA 0xC1
#endif


