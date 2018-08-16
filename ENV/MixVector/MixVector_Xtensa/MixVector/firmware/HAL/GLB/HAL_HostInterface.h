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
Filename     :                                           
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 

Modification History:
20140902     blakezhang     001 first create
*******************************************************************************/
#ifndef _HAL_HOSTINTERFACE_H
#define _HAL_HOSTINTERFACE_H


/* Sector size is 512 Bytes */
#define   SEC_SIZE_BITS          9
#define   SEC_SIZE              (1<<SEC_SIZE_BITS)
#define   SEC_SIZE_MSK          (SEC_SIZE - 1)

/* host memory page size is 4K Bytes */
#define   SEC_PER_HPAGE_BITS    (3)
#define   SEC_PER_HPAGE         (1<<SEC_PER_HPAGE_BITS)
#define   SEC_PER_HPAGE_MSK     (SEC_PER_HPAGE-1)

#define   HPAGE_SIZE_BITS         (SEC_SIZE_BITS+SEC_PER_HPAGE_BITS)
#define   HPAGE_SIZE              (1<<HPAGE_SIZE_BITS)
#define   HPAGE_SIZE_MSK          (HPAGE_SIZE-1)

#define SEC_PER_DATABLOCK       (g_ulSecPerDataBlk)
#define SEC_PER_DATABLOCK_BITS  (g_ulSecPerDataBlkBits)

#define MAX_FCQ       (32)

/* Host PRD and PRD table related defines */
#define MAX_PRDT_NUM  40
#define MAX_PRP_NUM   33

#ifdef HOST_NVME
#define MAX_SLOT_NUM           64
#define  COMMAND_HEADER_SIZE  (64)
#else   /* AHCI or SATA */
#define MAX_SLOT_NUM            32
#define COMMAND_HEADER_SIZE    (32)
#endif


#define COMMAND_TABLE_PRDT_SIZE  (16)
#define COMMAND_TABLE_OTHER_SIZE  (32*4)
#define COMMAND_TABLE_SIZE  (COMMAND_TABLE_OTHER_SIZE + (COMMAND_TABLE_PRDT_SIZE * MAX_PRDT_NUM))
#define HCT_S0_LENGTH       ( COMMAND_HEADER_SIZE * MAX_SLOT_NUM )
#define HCT_S1_BASE         ( HCT_SRAM_BASE + HCT_S0_LENGTH )

#define GET_PRD_ADDR(TAG, PRD_INDEX) (HCT_S1_BASE + ((TAG)*COMMAND_TABLE_SIZE) + ((PRD_INDEX)*COMMAND_TABLE_PRDT_SIZE) + COMMAND_TABLE_OTHER_SIZE)

#ifdef HOST_NVME
#define GET_PRP_ADDR(TAG,PRP_INDEX)  (HCT_S1_BASE + ((TAG) * MAX_PRP_NUM + (PRP_INDEX)) * sizeof(PRP))
#endif

/* Format of a PRD entry builded by host. */
typedef struct _prd{
        U32     DBALo;
        U32     DBAHi;
        U32     Rsvd1;
        U32     DBC:22;
        U32     Rsvd2:9;
        U32     I:1;    // Interrupt on Completion
}PRD, *PPRD;


typedef struct _prp
{
    union
    {
        struct
        {
            U32 ulDW0;
            U32 ulDW1;
        };

        struct
        {
            U32 rsv:2;
            U32 Offset:10;
            U32 PBAL:20;
            U32 PBAH;
        };
    };
}PRP,*PPRP;


extern GLOBAL U32 g_ulSecPerDataBlk;
extern GLOBAL U32 g_ulSecPerDataBlkBits;

#endif

