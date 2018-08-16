/****************************************************************************
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
*****************************************************************************
Filename     : L0_Config.h
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20141023     BlakeZhang     001 first create
****************************************************************************/
#ifndef _L0_CONFIG_H
#define _L0_CONFIG_H

#include "HAL_FlashChipDefine.h"
#include "HAL_HostInterface.h"

#ifndef SEC_PER_LPN
#define   SEC_PER_LPN_BITS        3
#define   SEC_PER_LPN            (1<<SEC_PER_LPN_BITS)
#define   SEC_PER_LPN_MSK        (SEC_PER_LPN -1)
#endif

#ifndef   BUF_SIZE
#define   BUF_SIZE_BITS         LOGIC_PIPE_PG_SZ_BITS
#define   BUF_SIZE              (1 << BUF_SIZE_BITS)      
#define   BUF_SIZE_MSK          (BUF_SIZE - 1)
#endif

#ifndef   SEC_PER_BUF
#define   SEC_PER_BUF_BITS      (BUF_SIZE_BITS - SEC_SIZE_BITS)
#define   SEC_PER_BUF           (1<<SEC_PER_BUF_BITS)
#define   SEC_PER_BUF_MSK       (SEC_PER_BUF -1)
#endif

#if 0
enum {
    FW_UPGRADE_PAYLOAD_TYPE_SPI_IMG  ,
    FW_UPGRADE_PAYLOAD_TYPE_BL_IMG   ,
    FW_UPGRADE_PAYLOAD_TYPE_FW_IMG  ,
    FW_UPGRADE_PAYLOAD_TYPE_IMG_NUM
};

typedef union _FW_UPGRADE_PAYLOAD_ENTRY
{
    struct
    {
        /* BYTE 0: Config Bits */
        U32 bsPayLoadEn : 1;  /* 1: enable; 0: disable */
        U32 bsPayLoadType : 3;
        U32 bsPayLoadParam : 4; /* additional Param for each type */

        /* BYTE 1-3: length in 512 Bytes */
        U32 ulPayLoadSecLength : 24;

        /* DW1: payload CRC checksum */
        U32 ulPayLoadCRC;
    };

    U32 ulSelFlag[2];
} FW_UPGRADE_PAYLOAD_ENTRY;

#define FW_UPGRADE_PAYLOAD_ENTRY_CNT    (16)
#define FW_UPGRADE_IMG_HEAD_SIZE        (512)          

typedef union _FW_UPGRADE_HEAD_ENTRY
{
    struct{
        FW_UPGRADE_PAYLOAD_ENTRY aPayLoadEntry[FW_UPGRADE_PAYLOAD_ENTRY_CNT];
        U32 ulPad[95];
        U32 ulHeadCRC;
    };

    U32 ulDW[FW_UPGRADE_IMG_HEAD_SIZE>>2];
} FW_UPGRADE_HEAD_ENTRY;

typedef struct _FW_UPGRADE_PAYLOAD_INFO
{
    U32 ulPayloadEn;
    U32 ulPayloadAddrInDram;
    U32 ulPayloadLenByte;
}FW_UPGRADE_PAYLOAD_INFO;
#endif

//#define FW_IMAGE_MEM_SIZE   (768u * 1024u)
#define FW_IMAGE_MEM_SIZE   (832u * 1024u)

typedef struct _FW_Update
{
    U32 ulFwBaseAddr;
    U32 ulFwSize;
    U32 ulDldSize;
    BOOL bUpdateFwSize;
    //FW_UPGRADE_PAYLOAD_INFO aPayloadInfo[FW_UPGRADE_PAYLOAD_TYPE_IMG_NUM];
#ifdef HOST_SATA
    U32 ulPreOffset;
    U32 ulPreDataSecCnt;
#endif
}FW_UPDATE;

//VT3533 FW IMAGE : each image starting address is 64B aligned

#define HEADER_SIZE                     1024
#define BOOT_MAGIC_SIZE                 16

typedef struct _IMAGE_HDR {
    U8 magic[BOOT_MAGIC_SIZE];

    U32 tinyloader_size;  /* size in bytes */
    U32 tinyloader_addr;  /* physical load addr */
    U32 tinyloader_exec; /* tinyloader exec addr */
    U32 tinyloader_crc32;

    U32 bootloader_size;  /* size in bytes */
    U32 bootloader_addr;  /* physical load addr */
    U32 bootloader_exec; /* bootloader exec addr */
    U32 bootloader_crc32;

    U32 mcu0fw_size;  /* size in bytes */
    U32 mcu0fw_addr;  /* physical load addr */
    U32 mcu0fw_exec; /* mcu0 exec addr */
    U32 mcu0fw_crc32;

    U32 mcu1fw_size;  /* size in bytes */
    U32 mcu1fw_addr;  /* physical load addr */
    U32 mcu1fw_exec; /* mcu1 exec addr */
    U32 mcu1fw_crc32;

    U32 mcu2fw_size;  /* size in bytes */
    U32 mcu2fw_addr;  /* physical load addr */
    U32 mcu2fw_exec; /* mcu2 exec addr */
    U32 mcu2fw_crc32;

    U32 rom_size;  /* size in bytes */
    U32 rom_addr;  /* physical load addr */
    U32 rom_exec; /* rom exec addr */
    U32 rom_crc32;
} IMAGE_HDR;

#endif
