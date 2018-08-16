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
20160504    abby    create
*******************************************************************************/
#ifndef __HAL_FLASHCHIP_DEFINE_H__
#define __HAL_FLASHCHIP_DEFINE_H__

#include "BaseDef.h"

/* flash chip parameter define */
#define FLASH_3D_MLC
#define FLASH_TSB                      // only for flash-model or FlashChip notes.
#define TOGGLE20                       // only support TOGGLE2.0

/*  LDPC code sel for this type flash  */
#define LDPC_MAT_MODE               (0)       //0 : LDPC_MAT_MODE_0   136 72   88   104
                                              //1 : LDPC_MAT_MODE_1   136 112  120  128
#define LDPC_MAT_SEL_FST_15K        (1)  // for first 15K user data, TSB 3D MLC sel code1, parity 72 Bytes
#define LDPC_MAT_SEL_LASE_1K        (2)  // for last 1K user data + Red, TSB 3D MLC sel code2, parity 88 Bytes
#define LDPC_MAT_PRT_LEN_FST_15K    (g_atLdpcMatTable[LDPC_MAT_MODE][LDPC_MAT_SEL_FST_15K])
#define LDPC_MAT_PRY_LEN_LAST_1K    (g_atLdpcMatTable[LDPC_MAT_MODE][LDPC_MAT_SEL_LASE_1K])

/*  LUN related */
#define LUN_PER_CE_BITS             0
#define LUN_PER_CE                  (1 << LUN_PER_CE_BITS)

/*  plane related */
#define PLN_PER_LUN_BITS            1
#define PLN_PER_LUN                 (1 << PLN_PER_LUN_BITS)

#define PHY_PLN_PER_LUN_BITS        PLN_PER_LUN_BITS
#define PHY_PLN_PER_LUN             (1 << PHY_PLN_PER_LUN_BITS)
#define PHY_PLN_PER_LUN_MSK         (PHY_PLN_PER_LUN - 1)

/*  block related   */

#if (defined(SIM) && (defined(CE_DECODE) || (LUN_PER_CE_BITS>0)))
#define BLK_PER_PLN_BITS            10
#else
#define BLK_PER_PLN_BITS            11
#endif
#define BLK_PER_PLN                 1366

#define NORMAL_BLK_MARK             0xFF
#define BAD_BLK_MARK_COLUMN_POS     16384
#define RSV_BLK_PER_PLN             98

/*  page related  */
#define PG_PER_BLK_BITS             9                       //only 384 pages
#define PG_PER_BLK                  (1 << PG_PER_BLK_BITS)  

#define PHY_PG_PER_BLK_BITS         PG_PER_BLK_BITS
#define PHY_PG_PER_BLK              (1 << PHY_PG_PER_BLK_BITS)
#define PHY_PG_PER_BLK_MSK          (PHY_PG_PER_BLK - 1)

//#define PG_PER_WL                   2

#define PGADDR_PER_PRG              2
#define INTRPG_PER_PGADDR           1

#define PHYPG_SZ_BITS               14                      // 16KB page
#define PHYPG_SZ                    (1 << PHYPG_SZ_BITS)

/*  sector related  */
#define SEC_SZ_BITS                 9                       //512B
#define SEC_PER_PG_BITS             (PHYPG_SZ_BITS - SEC_SZ_BITS)
#define SEC_PER_PG                  (1 << SEC_PER_PG_BITS)
#define SEC_PER_PG_MSK              (SEC_PER_PG -1)

/*  red related  */
#define RED_SZ_BITS                 6                       //64B
#define RED_SZ                      (1 << RED_SZ_BITS)
#define RED_SZ_DW                   (RED_SZ >> 2)
#define RED_SW_SZ_DW                (RED_SZ_DW << PHY_PLN_PER_LUN_BITS)

/*  pipe page related  */
#define PIPE_PG_SZ_BITS             (PHYPG_SZ_BITS + PLN_PER_LUN_BITS)
#define PIPE_PG_SZ                  (1 << PIPE_PG_SZ_BITS)
#define PIPE_PG_SZ_MSK              (PIPE_PG_SZ - 1)

#define PHY_PIPE_PG_SZ_BITS         (PHYPG_SZ_BITS + PHY_PLN_PER_LUN_BITS)
#define PHY_PIPE_PG_SZ              (1 << PHY_PIPE_PG_SZ_BITS)
#define PHY_PIPE_PG_SZ_MSK          (PHY_PIPE_PG_SZ - 1)

/*  scramble mask   */
#define SCRAMBLE_MSK_EN             0   //0-not mask,enable scramble in TSB flash

// define PRCQ DataIn/DataOut Size, only for flash model, added by jasonguo 20150618
#ifdef SIM
#define LOGIC_PG_NUM_BITS           0
#define LOGIC_PG_NUM                (1<<LOGIC_PG_NUM_BITS)

#define LOGIC_PG_SZ_BITS            (PHYPG_SZ_BITS - LOGIC_PG_NUM_BITS)
#define LOGIC_PG_SZ                 (1 << LOGIC_PG_SZ_BITS)
#define LOGIC_PG_SZ_MSK             (LOGIC_PG_SZ - 1)

#define LOGIC_SEC_PER_PG_BITS       (LOGIC_PG_SZ_BITS - SEC_SZ_BITS)
#define LOGIC_SEC_PER_PG            (1 << LOGIC_SEC_PER_PG_BITS)
#define LOGIC_SEC_PER_PG_MSK        (LOGIC_SEC_PER_PG -1)

#define LOGIC_RED_SZ_DW             (RED_SZ_DW/LOGIC_PG_NUM)

#define LOGIC_PLN_PER_LUN_BITS      (PHY_PLN_PER_LUN_BITS + LOGIC_PG_NUM_BITS)
#define LOGIC_PLN_PER_LUN           (1 << LOGIC_PLN_PER_LUN_BITS)
#define LOGIC_PLN_PER_LUN_MSK       (LOGIC_PLN_PER_LUN - 1)
#endif

#define PRG_CYC_CNT                 1  // program cycle in one WL
/*  flash ROW addr bit location  */
#define PLN_BIT_IN_ROW_ADDR         (PHY_PG_PER_BLK_BITS)
#define BLK_BIT_IN_ROW_ADDR         (PLN_BIT_IN_ROW_ADDR + PHY_PLN_PER_LUN_BITS)
#if (defined(SIM) && (defined(CE_DECODE) || (LUN_PER_CE_BITS>0)))
#define LUN_BIT_IN_ROW_ADDR         (BLK_BIT_IN_ROW_ADDR + BLK_PER_PLN_BITS + 1)
#else
#define LUN_BIT_IN_ROW_ADDR         (BLK_BIT_IN_ROW_ADDR + BLK_PER_PLN_BITS)
#endif
/*  MLC PAIR PAGE   */
#define PAIR_PAGE_INTERVAL_MAX      (1)//pending

typedef enum _PAIR_PAGE_TYPE
{
    LOW_PAGE = 1,
    HIGH_PAGE,
    LOW_PAGE_WITHOUT_HIGH
}PAIR_PAGE_TYPE;

#endif/*__HAL_FLASHCHIP_DEFINE_H__*/

