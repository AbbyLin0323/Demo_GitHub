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
*******************************************************************************/

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashCmd.h"
#include "HAL_FlashDriverBasic.h"

extern GLOBAL MCU12_VAR_ATTR volatile NFC_PRCQ_QE *g_pNfcPRCQQe;

/* PRCQ One Phase Cmd Element entry */
GLOBAL MCU12_VAR_ATTR U8 g_aCQE1E[QE_CMD_GROUP_DEPTH] = 
{
    0xEF,0xEE,0xFF,0xFA,
    0x90,0x00,0x30,0x60,
    0xD0,0x31,0x3F,0x85,
    0xFC,0xD4,0xD5,0xFF
};

/* PRCQ Two Phase Cmd Element entry */
GLOBAL MCU12_VAR_ATTR U16 g_aCQE2E[QE_CMD_GROUP_DEPTH] = 
{
    0x3000,0x1080,0x1181,0xD060,
    0x1180,0x1081,0xe005,0xe006,
    0x3060,0x1580,0x3200,0x3100,
    0x3160,0x1581,0xffff,0xffff
};

/* PRCQ Status Phase Cmd Element entry */
GLOBAL MCU12_VAR_ATTR U16 g_aSQEE[QE_OPRATION_GROUP_DEPTH] = 
{  
    0x3E70,0x2E70,0x3d70,0x0E70,
    0xbe70,0x3d78,0x2E78,0x3E78
};

/* PRCQ table */
GLOBAL MCU12_VAR_ATTR PRCQ_TABLE g_aPrcqTable[NF_PRCQ_CNT] =
{
    /* normal cmd's bsQEPhase area is initialize at run-time, unit by Byte*/
    {NF_PRCQ_READID,                    0, FALSE,  TRIG_CMD_OTHER,   0, INVALID_8F},
    {NF_PRCQ_ERS_1PLN,                  0, FALSE,  TRIG_CMD_ERASE,   0, INVALID_8F},
    {NF_PRCQ_READ_1PLN,                 0, FALSE,  TRIG_CMD_READ,    0, INVALID_8F},
    {NF_PRCQ_PRG_1PLN,                  0, FALSE,  TRIG_CMD_WRITE,   0, INVALID_8F}, 
    {NF_PRCQ_RESET,                     0, FALSE,  TRIG_CMD_OTHER,   0, INVALID_8F},
    {NF_PRCQ_ERS_2PLN,                  0, FALSE,  TRIG_CMD_ERASE,   0, INVALID_8F},
    {NF_PRCQ_READ_2PLN,                 0, FALSE,  TRIG_CMD_READ,    0, INVALID_8F}, 
    {NF_PRCQ_PRG_2PLN,                  0, FALSE,  TRIG_CMD_WRITE,   0, INVALID_8F},
    {NF_PRCQ_SETFEATURE,                0, FALSE,  TRIG_CMD_WRITE,   0, INVALID_8F},
    {NF_PRCQ_GETFEATURE,                0, FALSE,  TRIG_CMD_OTHER,   0, INVALID_8F},   
    {NF_PRCQ_CHANGEREADCOLUM_1PLN,      0, FALSE,  TRIG_CMD_READ,    0, INVALID_8F},
    {NF_PRCQ_CHANGEREADCOLUM_2PLN,      0, FALSE,  TRIG_CMD_READ,    0, INVALID_8F}, 
    
    /* pio cmd's bsQEPhase area is initialize when compile, unit by DW*/
    {NF_PRCQ_PIO_SETFEATURE,            6, TRUE,   TRIG_CMD_OTHER,   0, INVALID_8F},
    {NF_PRCQ_PIO_SETFEATURE_EX,         6, TRUE,   TRIG_CMD_OTHER,   0, INVALID_8F},
};

/* normal cmd PRCQ QE */
GLOBAL MCU12_VAR_ATTR U8 l_aQETable[] = 
{
    //NF_RCMD_INDEX_READID
    // 90 | addr x 1 | reg read |finish
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_90),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_ADDR)    |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_REG_READ)          |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END), 

    // NF_RCMD_INDEX_ERS_1PLN
    // 60 | addr X 3 (row addr)    |  d0  | bsy | finish 
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_D060),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END), 

    // NF_RCMD_INDEX_READ_1PLN
    // 00 | addr x 5 | 30  | bsy | 00 |data (pln0) | finish 
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),     

    // NF_RCMD_INDEX_PRG_1PLN
    // 80 |  addr x 5 | data(pln0) |10| bsy | finish
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1080),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),  
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),     

    // NF_RCMD_INDEX_RESET  
    // FF  | bsy | finish 
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_FF),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70), 
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),     

    // NF_RCMD_INDEX_ERS_2PLN
    // 60 | RowAdd X 3 | 60 | RowAdd X 3 | D0 | BSY|END  
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_60),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_D060),
    QE_GRP_ATTR(PRCQ_ROW_ADDR)          |QE_INDEX(0x1),   
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),  
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),         

    // NF_RCMD_INDEX_READ_2PLN,
    // 00 | Addr 5 |00| Addr 5 |30|BSY |
    // 06|ADD X5 |e0|Data pln 0|
    // 06|ADD X5 |e0|Data pln 1|   END    
#ifdef FLASH_CACHE_OPERATION
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3200),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_2E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SELECT),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3100),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN1),  
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(RETURN),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(JUMP),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70) ,
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SKIP),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_3F),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
#else
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3200),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_2E70),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN1),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),


#endif
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(QE_PLN0),
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(2),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(QE_PLN0), 
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN1),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(QE_PLN1),
    QE_GRP_ATTR(PRCQ_IDLE)              |QE_INDEX(2),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(QE_PLN1),    
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),
                                        
    // NF_RCMD_INDEX_PRG_2PLN           
#ifdef FLASH_CACHE_OPERATION            
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1180),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN0),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(QE_PLN0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_2E70),    
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1581),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(QE_PLN1),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(QE_PLN1),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(SELECT_W),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3D70),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),    
#else                                   
    // 80|ADD[0] X 5 | Data pln 0 |11 | BSY| 81 |ADD[1] X 5 | Data pln 1 | 10| BSY |END 
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1180),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(0),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_2E70),    
    QE_GRP_ATTR(PRCQ_THREE_PHASE_CMD)   |QE_INDEX(CQE2E_1081),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(1),
    QE_GRP_ATTR(PRCQ_DMA_WRITE)         |QE_INDEX(1),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),  
#endif
    // NF_RCMD_INDEX_SETFERAUTE
    // ef | addr x 1 | data(reg write) | finish
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_EF),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_ADDR)    |QE_INDEX(0),
    QE_GRP_ATTR(PRCQ_REG_WRITE)         |QE_INDEX(0x0), 
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),      

    // NF_RCMD_INDEX_GETFEATURE
    // EE/ADDR X1 |BSY |DATA(REG READ) |FINISH
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_EE),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_ADDR)    |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_GRP_ATTR(PRCQ_REG_READ)          |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END),    

    // NF_RCMD_INDEX_CHANGEREADCOLUM_1PLN
    //     05 | addr x 2 (col addr)|e0        | data (pln 0) | finish     
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(0),
    QE_GRP_ATTR(PRCQ_DMA_READ)         |QE_INDEX(0x0), 
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END), 

    // NF_RCMD_INDEX_CHANGEREADCOLUM_2PLN
    // 00 5x addr 05 2x addr e0  data
    // 00 5x addr 05 2x addr e0  data
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(0x0),
    QE_GRP_ATTR(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_GRP_ATTR(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(0x1),
    QE_GRP_ATTR(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_E005),
    QE_GRP_ATTR(PRCQ_COL_ADDR)          |QE_INDEX(0x1),
    QE_GRP_ATTR(PRCQ_DMA_READ)          |QE_INDEX(0x1),
    QE_GRP_ATTR(PRCQ_FINISH)            |QE_INDEX(END)
};

/* pio cmd PRCQ QE */
GLOBAL MCU12_VAR_ATTR U32 l_aPioCmdTable[] = 
{
    //NF_RCMD_INDEX_PIO_SETFEATURE,   
    RAW_PIO_CMD|PIO_CFG_VALUE(0xef),
    RAW_PIO_ADDR|PIO_CFG_VALUE(0x1),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x15),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0),
    RAW_PIO_DATAOUT|PIO_CFG_VALUE(0x0)|RAW_PIO_IO_LAST_PHS(1),

    //NF_RCMD_INDEX_PIO_SETFEATURE_EX
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
void MCU12_DRAM_TEXT HAL_NfcQEEInit(void)
{
    U32 ulQeIndex;

    for (ulQeIndex = 0; ulQeIndex < QE_CMD_GROUP_DEPTH; ulQeIndex++)
    {
        g_pNfcPRCQQe->aPRCQ_CQE1E[ulQeIndex] = g_aCQE1E[ulQeIndex];
        g_pNfcPRCQQe->aPRCQ_CQE2E[ulQeIndex] = g_aCQE2E[ulQeIndex];
    }

    for (ulQeIndex = 0; ulQeIndex < QE_OPRATION_GROUP_DEPTH; ulQeIndex++)
    {
        g_pNfcPRCQQe->aPRCQ_SQEE[ulQeIndex] = g_aSQEE[ulQeIndex];
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
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcPrcqTableInit(void)
{
    U8 ucCmdIndex;
    U32 ulQeIndex = 0;
    U32 ulCurQECnt = 0;
    U32 ulCurPioQECnt = 0;
    
    for (ucCmdIndex = 0; ucCmdIndex < NF_PRCQ_NON_PIO_NUM; ucCmdIndex++)
    {        
        for (ulCurQECnt = 0; (QE_GRP_ATTR(PRCQ_FINISH)|QE_INDEX(END)) != l_aQETable[ulQeIndex]; ulCurQECnt++,ulQeIndex++)
        {
            ;
        }
        ulQeIndex++;
        g_aPrcqTable[ucCmdIndex].bsQEPhase = ++ulCurQECnt;
    }
    
    ulCurQECnt = 0;
    for (ucCmdIndex = 0; ucCmdIndex < NF_PRCQ_CNT; ucCmdIndex++)
    {
        if (FALSE == g_aPrcqTable[ucCmdIndex].bsIsPIO)
        {
            g_aPrcqTable[ucCmdIndex].bsQEPtr = (U32)&l_aQETable[ulCurQECnt];
            ulCurQECnt += g_aPrcqTable[ucCmdIndex].bsQEPhase;
        }
        else
        {
            g_aPrcqTable[ucCmdIndex].bsQEPtr = (U32)&l_aPioCmdTable[ulCurPioQECnt];
            ulCurPioQECnt += g_aPrcqTable[ucCmdIndex].bsQEPhase;
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetPrcqEntry
Description: 
    get a prcq entry address.
Input Param:
    U8 ucPrcqType: equal to cmd index.
Output Param:
    none
Return Value:
    PRCQ_TABLE*: pointer to a prcq entry address.
Usage:
    FW call this function to get a prcq entry address.
History:
    20140902    tobey   uniform coding style.
------------------------------------------------------------------------------*/
PRCQ_TABLE* HAL_NfcGetPrcqEntry(U8 ucPrcqType)
{
    return &g_aPrcqTable[ucPrcqType];
}

/* end of file HAL_FlashCmd.c */

