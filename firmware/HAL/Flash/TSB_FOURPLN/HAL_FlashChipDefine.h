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
Filename    : HAL_FlashChipDefine.h
Version     : Ver 1.0
Author      : peter
Date        : 2012.01.18
Description : this file declare flash chip relative attribute and prcq initi-
              lization interface.
Others      :
Modify      :
20140904    tobey   uniform code style and wipe off unrelative code
20160210    abby    modify to meet VT3533
20160811    abby    optmization
*******************************************************************************/
#ifndef __HAL_FLASHCHIP_DEFINE_H__
#define __HAL_FLASHCHIP_DEFINE_H__

#include "BaseDef.h"

/* flash chip parameter define */
#define FLASH_TSB

/*************************/
/* Bad Block Mark Define */
/*************************/
#define NORMAL_BLK_MARK             (0xFF)
#define BAD_BLK_MARK_COLUMN_POS     (16384)

/************************************/
/* Nand Flash Memory Organization   */
/************************************/
// LUN Address
#define LUN_PER_CE_BITS             (0)
#define LUN_PER_CE                  (1 << LUN_PER_CE_BITS)

// PLN Address
#define PLN_PER_LUN_BITS            (2)
#define PLN_PER_LUN                 (1 << PLN_PER_LUN_BITS)
#define PLN_PER_LUN_MSK             (PLN_PER_LUN - 1)

// Block Address
#define BLK_PER_PLN_BITS            (11)
#define BLK_PER_PLN_MSK             ((1 << BLK_PER_PLN_BITS) - 1)
#define BLK_PER_PLN                 (1024)
#define RSV_BLK_PER_PLN             (39)

// Page/Wl Address
#define PG_PER_BLK_BITS             (8)
#define PG_PER_BLK                  (1 << PG_PER_BLK_BITS)

// Row Address = LUN Address + Block Address + PLN Address + Page Address
#define PLN_POS_IN_ROW_ADDR         (PG_PER_BLK_BITS)
#define BLK_POS_IN_ROW_ADDR         (PLN_POS_IN_ROW_ADDR + PLN_PER_LUN_BITS)
#define LUN_POS_IN_ROW_ADDR         (BLK_POS_IN_ROW_ADDR + BLK_PER_PLN_BITS)

/*  calcu flash row addr  */
#define HAL_FLASH_ROW_ADDR(Lun,Pln,Blk,Page)      (((Lun) << LUN_POS_IN_ROW_ADDR)|((Blk) << BLK_POS_IN_ROW_ADDR) \
                                                |((Pln) << PLN_POS_IN_ROW_ADDR)|(Page))
/*  calcu flash row addr  */
#define HAL_FLASH_SLC_ROW_ADDR(Lun,Pln,Blk,Page) (((Lun) << LUN_POS_IN_ROW_ADDR)|((Blk) << BLK_POS_IN_ROW_ADDR) \
                                                |((Pln) << PLN_POS_IN_ROW_ADDR)|(Page))
                                                
/***********************************/
/* Nand Flash Operation related    */
/***********************************/
#define SEC_SZ_BITS                 (9)   // 512B = LBA
#define PHYPG_SZ_BITS               (14)  // the min-physical page size
#define PHYPG_SZ                    (1 << PHYPG_SZ_BITS)

#define SEC_PER_PHYPG_BITS          (PHYPG_SZ_BITS - SEC_SZ_BITS)
#define SEC_PER_PHYPG               (1 << SEC_PER_PHYPG_BITS)
#define SEC_PER_PHYPG_MSK           (SEC_PER_PHYPG - 1)

#define PRG_CYC_CNT                 (1)  // program cycles for completing one row address
#define PGADDR_PER_PRG              (1)
#define INTRPG_PER_PGADDR           (1)

/*****************************/
/*  FTL User define related  */
/*****************************/
#define LOGIC_PG_PER_BLK            ((PG_PER_BLK / PGADDR_PER_PRG) * INTRPG_PER_PGADDR)
#define LOGIC_PG_PER_BLK_BITS       (PG_PER_BLK_BITS)//pending

#define LOGIC_PG_SZ_BITS            (PHYPG_SZ_BITS)
#define LOGIC_PG_SZ                 (1 << LOGIC_PG_SZ_BITS)

#define LOGIC_PIPE_PG_SZ_BITS       (LOGIC_PG_SZ_BITS + PLN_PER_LUN_BITS)
#define LOGIC_PIPE_PG_SZ_MSK        ((1 << LOGIC_PIPE_PG_SZ_BITS) - 1)
#define LOGIC_PIPE_PG_SZ            (1 << LOGIC_PIPE_PG_SZ_BITS)

#define SEC_PER_LOGIC_PG_BITS       (LOGIC_PG_SZ_BITS - SEC_SZ_BITS)
#define SEC_PER_LOGIC_PG            (1 << SEC_PER_LOGIC_PG_BITS)

#define SEC_PER_LOGIC_PIPE_PG_BITS  (LOGIC_PIPE_PG_SZ_BITS - SEC_SZ_BITS)
#define SEC_PER_LOGIC_PIPE_PG       (1 << SEC_PER_LOGIC_PIPE_PG_BITS)

/***************************/
/* Flash Page Layout Define*/
/***************************/
/*  LDPC code sel for this type flash  */
#define LDPC_MAT_MODE               (0)  // 0 : LDPC_MAT_MODE_0 = {136, 72,  88,  104}
                                         // 1 : LDPC_MAT_MODE_1 = {136, 112, 120, 128}
#define LDPC_MAT_SEL_FST_15K        (1)  // for first 15K user data, 15nm TSB 4PLNsel code1, parity  72 Bytes
#define LDPC_MAT_SEL_LASE_1K        (0)  // for last 1K user data + Red, 15nm TSB 4PLNsel code0, parity 136 Bytes
#define LDPC_MAT_PRT_LEN_FST_15K    (g_atLdpcMatTable[LDPC_MAT_MODE][LDPC_MAT_SEL_FST_15K])
#define LDPC_MAT_PRY_LEN_LAST_1K    (g_atLdpcMatTable[LDPC_MAT_MODE][LDPC_MAT_SEL_LASE_1K])

// the first 15CW is 1024B+2B(CRC)+4B(LBA)/Per4K+72B(ECC), the last CW is 1024B+2B(CRC)+4B(LBA)+64B(RED)+136B(ECC).
#define EMPTY_THRD_FST_15K         (8702)   // 1024B + 2B(CRC) + 4B(LBA)/Per4K + 104B(ECC), default 99% => ((1024+2+128)*15 + 4*3)*8*0.99/15 = 9146.016
#define EMPTY_THRD_LAST_1K         (9830)   // 1024B + 2B(CRC) + 4B(LBA) + 64B(RED) + 136B(ECC), N0 is 10 => ((1024+2+4+64+136)*8) - 10 = 9830  (3D_TLC : Real worst case : 9835)

// Flash Red Define
#define RED_SZ_BITS                 (6)   // Default 64Byte Red_Size per physical page.
#define RED_SZ                      (1 << RED_SZ_BITS)
#define RED_SZ_DW                   (RED_SZ >> 2)
#define RED_SW_SZ_DW                (RED_SZ_DW << PLN_PER_LUN_BITS)
#define RED_PRG_SZ                  ((RED_SW_SZ_DW << 2) * INTRPG_PER_PGADDR * PGADDR_PER_PRG)

// Scramble mask Define
#define SCRAMBLE_MSK_EN             (0)   //0-not mask,enable scramble in TSB flash

/*****************************/
/* Pair Page related */
/*****************************/
#define PAIR_PAGE_PER_BLK           (PG_PER_BLK / 2)
#define PAIR_PAGE_PRG_MAX           (1)
#define PAIR_PAGE_INTERVAL_MAX      (3)

typedef enum _PAIR_PAGE_TYPE
{
    LOW_PAGE = 1,
    HIGH_PAGE,
    LOW_PAGE_WITHOUT_HIGH
}PAIR_PAGE_TYPE;

#endif/*__HAL_FLASHCHIP_DEFINE_H__*/

