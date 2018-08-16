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
20140904    tobey     uniform code style and wipe off unrelative code
*******************************************************************************/

#ifndef __HAL_FLASHCHIP_DEFINE_H__
#define __HAL_FLASHCHIP_DEFINE_H__

#include "BaseDef.h"

#define FLASH_L85

/* flash chip parameter define */
#define BAD_BLK_MARK_POS          16384
#define RSVD_BLK_PER_CE           0
#define BIT_LUN_IN_ROW_ADDR       20
#define PG_PER_BLK_BITS           9

#define DUAL_PLANE_ENABLE         1
#define FOUR_PLANE_ENABLE         0
#define SINGLE_PLANE_ENALBE       0

/* General Chip define */
#if (DUAL_PLANE_ENABLE==1)
#define PLN_PER_PU_BITS           1
#elif (FOUR_PLANE_ENABLE==1)
#define PLN_PER_PU_BITS           2
#elif (SINGLE_PLANE_ENALBE==1)
#define PLN_PER_PU_BITS           0
#endif

#define PLN_PER_PU                (1 << PLN_PER_PU_BITS)
#define PLN_PER_PU_MSK            (PLN_PER_PU - 1)

#define SEC_PER_PG_BITS           5
#define SEC_PER_PG                (1 << SEC_PER_PG_BITS)
#define SEC_PER_PG_MSK            (SEC_PER_PG -1)

#define PG_PER_BLK                (1 << PG_PER_BLK_BITS)    
#define PG_PER_BLK_MSK            (PG_PER_BLK - 1)

#define BLK_PER_PLN_BITS          10
#define BLK_PER_PLN               (1 << BLK_PER_PLN_BITS)
#define BLK_PER_PLN_MSK           (BLK_PER_PLN - 1)
#define BLK_PER_CE                BLK_PER_PLN

#define PG_SZ_BITS                14
#define PG_SZ                     (1 << PG_SZ_BITS)
#define PG_SZ_MSK                 (PG_SZ - 1)

#define RED_SZ_BITS               6
#define RED_SZ                    (1 << RED_SZ_BITS)
#define RED_SZ_MSK                (RED_SZ - 1)
#define RED_SZ_DW                 (RED_SZ >> 2)

#define RED_SW_SZ_DW              (RED_SZ_DW*PLN_PER_PU)
#define RED_SZ_PER_MCU            (OTFB_RED_DATA_MCU2_BASE-OTFB_RED_DATA_MCU1_BASE)

#define PIPE_PG_SZ_BITS           (PG_SZ_BITS + PLN_PER_PU_BITS)
#define PIPE_PG_SZ                (1 << PIPE_PG_SZ_BITS)
#define PIPE_PG_SZ_MSK            (PIPE_PG_SZ - 1)

#define BLK_SZ_BITS               (PIPE_PG_SZ_BITS + PG_PER_BLK_BITS)
#define BLK_SZ                    (1 << BLK_SZ_BITS)
#define BLK_SZ_MSK                (BLK_SZ - 1)

#define LUN_NUM_BITS              0  //immutable L85 NAND flash only support 1 LUN
#define LUN_NUM                   (1 << LUN_NUM_BITS)

#define SCRAMBLE_MSK_EN           1

#endif/*__HAL_FLASHCHIP_DEFINE_H__*/

