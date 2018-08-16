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
Filename    : HAL_FlashCmd.c
Version     : Ver 1.0
Author      : peter
Date        : 2012.01.08
Description : this file encapsulate PRCQ CMD relative interface definitions.
Others      : This file's flash cmd is for L85&TSB MLC chip. except for one TLC's SLC
              mode 1 PLN read cmd.
Modify      :
20140904    tobey     uniform code style and wipe off unrelative code
20151221    abby      modify to meet VT3533
*******************************************************************************/
#include "HAL_FlashCmd.h"

GLOBAL MCU12_VAR_ATTR volatile NFC_PRCQ_QE *g_pNfcPRCQQe;
GLOBAL MCU12_VAR_ATTR volatile NFC_PRCQ_QE_EXT *g_pNfcPRCQQeExt;

/* PRCQ One Phase Cmd Element entry */
GLOBAL MCU2_DRAM_TEXT U8 g_aCQE1E[QE_CMD_GROUP_DEPTH] =
{
    0xEF,0xEE,0xFF,0xFA,
    0x90,0x00,0x30,0x60,
    0xD0,0x31,0x3F,0x85,
    0xFC,0xD4,0xD5,0xFF
};
#if 0//extend for future
GLOBAL MCU2_DRAM_TEXT U8 g_aCQE1EExt[QE_CMD_GROUP_DEPTH] =
{
    0xEF,0xEE,0xFF,0xFA,
    0x90,0x00,0x30,0x60,
    0xD0,0x31,0x3F,0x85,
    0xFC,0xD4,0xD5,0xFF
};
#endif
/* PRCQ Two Phase Cmd Element entry */
GLOBAL MCU2_DRAM_TEXT U16 g_aCQE2E[QE_CMD_GROUP_DEPTH] =
{
    0x3000,0x1080,0x1181,0xD060,
    0x1180,0x1081,0xe005,0xe006,
    0x3060,0x1580,0x3200,0x3100,
    0x3160,0x1581,0xffff,0xffff
};
#if 0//extend for future
GLOBAL MCU2_DRAM_TEXT U16 g_aCQE2EExt[QE_CMD_GROUP_DEPTH] =
{
    0x3000,0x1080,0x1181,0xD060,
    0x1180,0x1081,0xe005,0xe006,
    0x3060,0x1580,0x3200,0x3100,
    0x3160,0x1581,0xffff,0xffff
};
#endif
/* PRCQ Status Phase Cmd Element entry */
GLOBAL MCU2_DRAM_TEXT U16 g_aSQEE[QE_OPRATION_GROUP_DEPTH] =
{
    0x3E70,0x2E70,0x3d70,0x0E70,
    0xbe70,0x3d78,0x2E78,0x3E78
};

/* PRCQ table */
GLOBAL MCU12_VAR_ATTR PRCQ_TABLE g_aPrcqTable[FLASH_PRCQ_CNT] =
{
    /* normal cmd's */
    {FLASH_PRCQ_RESET,                         NF_NORMAL_CMD,  TRIG_CMD_OTHER,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_RESET_NOSTS,                   NF_NORMAL_CMD,  TRIG_CMD_OTHER,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_RESET_LUN,                     NF_NORMAL_CMD,  TRIG_CMD_OTHER,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_READID,                        NF_NORMAL_CMD,  TRIG_CMD_OTHER,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_SETFEATURE,                    NF_NORMAL_CMD,  TRIG_CMD_WRITE,   INVALID_BIT(9),    0},
    /*  MLC 1PLN cmd  */
    {FLASH_PRCQ_ERS_1PLN,                      NF_NORMAL_CMD,  TRIG_CMD_ERASE,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_READ_1PLN,                     NF_NORMAL_CMD,  TRIG_CMD_READ,    INVALID_BIT(9),    0},
    {FLASH_PRCQ_PRG_1PLN,                      NF_NORMAL_CMD,  TRIG_CMD_WRITE,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_CHANGEREADCOLUM_1PLN,          NF_NORMAL_CMD,  TRIG_CMD_READ,    INVALID_BIT(9),    0},
#ifndef BOOTLOADER
    /*  MLC FULL PLN cmd, L95 MLC is 2 PLN flash  */
    {FLASH_PRCQ_ERS_FULL_PLN,                  NF_NORMAL_CMD,  TRIG_CMD_ERASE,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_READ_FULL_PLN,                 NF_NORMAL_CMD,  TRIG_CMD_READ,    INVALID_BIT(9),    0},
    {FLASH_PRCQ_PRG_FULL_PLN,                  NF_NORMAL_CMD,  TRIG_CMD_WRITE,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_GETFEATURE,                    NF_NORMAL_CMD,  TRIG_CMD_OTHER,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_CHANGEREADCOLUM_FULL_PLN,      NF_NORMAL_CMD,  TRIG_CMD_READ,    INVALID_BIT(9),    0},
    {FLASH_PRCQ_READ_STS_ENHANCE,         NF_NORMAL_CMD,  TRIG_CMD_OTHER,   INVALID_BIT(9),    0}, 
#endif

    /* pio cmd's bsCmdDepth area is initialize when compile, unit by DW*/
    {FLASH_PRCQ_PIO_SETFEATURE,            NF_PIO_CMD,     TRIG_CMD_OTHER,   INVALID_BIT(9),    0},
    {FLASH_PRCQ_PIO_SETFEATURE_EX,         NF_PIO_CMD,     TRIG_CMD_OTHER,   INVALID_BIT(9),    0}
};

/* normal cmd PRCQ QE */
GLOBAL MCU12_DRAM_TEXT U8 l_aQETable[] =
{
    // FLASH_PRCQ_RESET 
    // FF  | bsy | finish
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_FF),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_RESET_NOSTS
    // FF  | finish
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_FF),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_RESET_LUN
    // FA | ADDR  | bsy | finish
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_FA),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),
    
    //FLASH_PRCQ_READID
    // 90 | addr x 1 | reg read |finish
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_90),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_ADDR)    |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_REG_READ)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_SETFEATURE
    // ef | addr x 1 | data(reg write) | finish
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_EF),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_ADDR)    |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_REG_WRITE)         |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_ERS_1PLN
    // 60 | addr X 3 (row addr) | d0 | bsy | finish
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_D060),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_READ_1PLN
    // 00 | addr x 5 | 30  | bsy | 00 | addr x 5 | 05 | addr x 5 | E0 | data (pln0) | finish
#ifdef FLASH_CACHE_OPERATION//pending, need vefify
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SELECT),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3100),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),  
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(RETURN),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(JUMP),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SKIP),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_3F),
#else
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
#endif
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(1),   // wait a mommont before data out to dram
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_RCMD_INDEX_PRG_1PLN
    // 80 |  addr x 5 | data(pln0) |10| bsy | finish
#ifdef FLASH_CACHE_OPERATION
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1580),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_2E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SELECT_W),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3D78),
#else
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1080),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(ADDR0),
#endif
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_CHANGEREADCOLUM_1PLN
    // 05 | addr x 2 (col addr)|e0      | data (pln 0) | finish
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(1),  
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

#ifndef BOOTLOADER
    // FLASH_RCMD_INDEX_ERS_FULL_PLN
    // 60 | RowAdd X 3 | 60 | RowAdd X 3 | D0 | BSY|END
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_60),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_D060),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR1),
    //read status of each plane
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_RCMD_INDEX_READ_FULL_PLN,
    // 00 | Addr 5 |00| Addr 5 |30|BSY |
    // 06|ADD X5 |e0|Data pln 0|
    // 06|ADD X5 |e0|Data pln 1|   END
#ifdef FLASH_CACHE_OPERATION
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3200),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_2E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SELECT),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3100),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(RETURN),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(JUMP),

    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SKIP),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_3F),
#else
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3200),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_2E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR1),
#endif
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR1),
    
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(1),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(ADDR0),

    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(1),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_RCMD_INDEX_PRG_FULL_PLN
#ifdef FLASH_CACHE_OPERATION
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1180),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_2E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1581),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(ADDR1),
    
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SELECT_W),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3D78),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SELECT_W),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3D78),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),
#else
    // 80|ADD[0] X 5 | Data pln 0 |11 | BSY| 81 |ADD[1] X 5 | Data pln 1 | 10| BSY |END
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1180),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_2E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1081),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(ADDR1),
    //read status of each plane
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR1),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),
#endif

    // FLASH_PRCQ_GETFEATURE
    // EE | ADDR X1 |BSY |DATA(REG READ) |FINISH
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_EE),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_ADDR)    |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_REG_READ)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_CHANGEREADCOLUM_FULL_PLN
    // 00 5x addr 05 2x addr e0  data
    // 00 5x addr 05 2x addr e0  data
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(1),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(ADDR0),

    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_2E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(0x0),

    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(ADDR1),    
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(ADDR1),    
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(1),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(ADDR1), 
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),

    // FLASH_PRCQ_READ_STS_ENHANCE_1PLN
    QE_GRP_ATTR(PRCQ_READ_STATUS_EH)    |QE_INDEX(SQEE_3E78),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(ADDR0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END)

#endif //ifndef BOOTLOADER
};

/* pio cmd PRCQ QE */
GLOBAL MCU2_DRAM_TEXT U32 l_aPioCmdTable[] =
{
    //FLASH_PRCQ_PIO_SETFEATURE
    RAW_PIO_CMD|PIO_CFG_VALUE(0xef),
    RAW_PIO_ADDR|PIO_CFG_VALUE(0x1),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x15),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0)|RAW_PIO_IO_LAST_PHS(1),

    //FLASH_PRCQ_PIO_SETFEATURE_EX
    RAW_PIO_CMD|PIO_CFG_VALUE(0xef),
    RAW_PIO_ADDR|PIO_CFG_VALUE(0x1),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x5),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0)|RAW_PIO_IO_LAST_PHS(1)
};

/*------------------------------------------------------------------------------
Name: HAL_NfcQEEInit
Description:
    Initialize PRCQ QE area. including 1 cycle cmd group, 2 cycle cmd group
    and operation group, details refer to PRCQ spec.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoked in BootLoader or FW. PRCQ QE area may be different between BootLoader and FW.
History:
    20140902    tobey   uniform coding style.
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_NfcQEEInit(void)
{
    U32 ulQeIndex;

    for (ulQeIndex = 0; ulQeIndex < QE_CMD_GROUP_DEPTH; ulQeIndex++)
    {
        g_pNfcPRCQQe->aPRCQ_CQE1E[ulQeIndex] = g_aCQE1E[ulQeIndex];
        g_pNfcPRCQQe->aPRCQ_CQE2E[ulQeIndex] = g_aCQE2E[ulQeIndex];
    }
    //extend CQEE, add in 533
    for (ulQeIndex = 0; ulQeIndex < QE_CMD_GROUP_DEPTH; ulQeIndex++)
    {
        g_pNfcPRCQQeExt->aPRCQ_CQE1E[ulQeIndex] = g_aCQE1E[ulQeIndex];//g_aCQE1EExt[ulQeIndex];//extend for future
        g_pNfcPRCQQeExt->aPRCQ_CQE2E[ulQeIndex] = g_aCQE2E[ulQeIndex];//g_aCQE2EExt[ulQeIndex];//extend for future
    }
    for (ulQeIndex = 0; ulQeIndex < QE_OPRATION_GROUP_DEPTH; ulQeIndex++)
    {
        g_pNfcPRCQQe->aPRCQ_SQEE[ulQeIndex] = g_aSQEE[ulQeIndex];
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcPrcqInit
Description: 
    Fill PRCQ sequency of all cmd into SRAM 
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoked in BootLoader or FW. PRCQ QE area may be different between BootLoader and FW.
History:
    20151027    abby create.
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcPrcqInit(void)
{
    U32 ulQeCnt = 0;
    U32 ulPioQeCnt = 0;
    U8 ucCmdCodeIndex = 0;
    
    /*  g_pNfcPRCQ ->PRCQ_ENTRY_BASE  */
    volatile U8 *pNfcQe = (volatile U8 *)g_pNfcPRCQ;
    
    /*  fill every QE into SRAM align in byte unit  */
    for (ucCmdCodeIndex = 0; ucCmdCodeIndex < FLASH_PRCQ_NON_PIO_NUM; ucCmdCodeIndex++)
    {
        do
        {
            *pNfcQe = l_aQETable[ulQeCnt];
            pNfcQe++;
            ulQeCnt++;

        }while ((QE_GRP_ATTR(PRCQ_FINISH)|QE_INDEX(END)) != l_aQETable[ulQeCnt-1]);
        
        /*  align in DW    */
        pNfcQe = (volatile U8 *)ALIGN_IN_DW((U32)((U32 *)pNfcQe));
    }
    
    /*  continue to fill PIO QE into SRAM by DW align  */
    g_pNfcPRCQ = (U32 *)pNfcQe;
    
    for (ucCmdCodeIndex = FLASH_PRCQ_NON_PIO_NUM; ucCmdCodeIndex < FLASH_PRCQ_CNT; ucCmdCodeIndex++)
    {
        do
        {
            *g_pNfcPRCQ = l_aPioCmdTable[ulPioQeCnt];
            g_pNfcPRCQ++;
            ulPioQeCnt++;

        } while ( RAW_PIO_IO_LAST_PHS(1) != (l_aPioCmdTable[ulPioQeCnt-1] & (0x1 << 31)) );
    }
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcPrcqTableInit
Description: 
    Initialize PRCQ table. specially bsQEPhase and bsQEPtr.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoked by HAL_NfcInterfaceInitBasic().
History:
    20140902    tobey   uniform coding style.
    20151105    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcPrcqTableInit(void)
{
    U8  ucCmdCodeIndex = 0;
    U32 ulPrcqDwCnt = 0;
    U32 ulByteCnt = 0;  //record byte location of normal cmd PRCQ
    U32 ulQECnt = 0;    //record QE number of normal cmd PRCQ
    U32 ulPioQECnt = 0;
    U32 ulBasePioStartDw = 0;

    /*  init normal cmd: 1 byte for 1 QE, need to align in DW    */
    for (ucCmdCodeIndex = 0; ucCmdCodeIndex < FLASH_PRCQ_NON_PIO_NUM; ucCmdCodeIndex++)
    {
        /*  get start PRCQ DW of this cmdCode   */
        g_aPrcqTable[ucCmdCodeIndex].bsPRCQStartDw = ulPrcqDwCnt;
        
        /*  ACC QE and byte number  */
        do 
        {
            ulByteCnt++;
            ulQECnt++;
        }while ( (QE_GRP_ATTR(PRCQ_FINISH)|QE_INDEX(END)) != l_aQETable[ulQECnt-1] );

        /* align in DW  */
        ulByteCnt = ALIGN_IN_DW(ulByteCnt);

        /*  Get next cmd start DW  */
        ulPrcqDwCnt = ulByteCnt / sizeof(U32);
    }

    /*  Set a base DW for ACC to avoid ulPioQECnt over l_aPioCmdTable[] definition*/
    ulBasePioStartDw = ulPrcqDwCnt;
    
    /*  init PIO cmd: 1 DW for 1 PIO QE, don't need to align in DW    */
    for (ucCmdCodeIndex = FLASH_PRCQ_NON_PIO_NUM; ucCmdCodeIndex < FLASH_PRCQ_CNT; ucCmdCodeIndex++)
    {
        /*  get start PRCQ DW of PIO cmdCode  */
        g_aPrcqTable[ucCmdCodeIndex].bsPRCQStartDw = ulBasePioStartDw + ulPioQECnt;
        
        /*  ACC PIO QE number  */
        do
        {
            ulPioQECnt++;
        }while (RAW_PIO_IO_LAST_PHS(1) != (l_aPioCmdTable[ulPioQECnt-1]&(0x1<<31)) );
                
        ulPrcqDwCnt = ulPioQECnt + ulBasePioStartDw;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetPrcqStartDw
Description: 
    get a prcq entry start DW;
    fuction instr put into SRAM
Input Param:
    U8 ucCmdType: equal to cmdcode in g_aPrcqTable.
Output Param:
    None
Return Value:
    PRCQ Start Dw
Usage:
    FW call this function to get a prcq start DW location for NFCQ.
History:
    20151028    abby   create.
------------------------------------------------------------------------------*/
U32 HAL_NfcGetPrcqStartDw(U8 ucCmdCode)
{
    return g_aPrcqTable[ucCmdCode].bsPRCQStartDw;
}

/* end of file HAL_FlashCmd.c */

