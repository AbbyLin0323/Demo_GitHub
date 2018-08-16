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
  File Name     : HAL_AhciHCT.h
  Version       : Initial Draft
  Author        : Kristin Wang
  Created       : 2013/9/9
  Last Modified :
  Description   : Definition of interface between L0 FW and HW, designed by SSD
  Function List :
  History       :
  1.Date        : 2013/9/9
    Author      : Charles Zhou
    Modification: Created file

******************************************************************************/
#ifndef _NVME_HCT_H
#define _NVME_HCT_H

#include "HAL_Inc.h"
#include "HAL_HCT.h"
#include "L0_Interface.h"

/*SN*/
#define SN_COMMAND_HEADER       (0)
#define SN_PRPT                 (1)


/*
 * S0   Command Header  , 32 * 8DW      , 1k
 * S1   CFIS + PRDT     , 32 * 768      , 24k
 * S2   ---
 * S3   RFIS            , 32 * 0x60     , 3k
 * S4   ---
 *
 * The total length of S0~S4 is about 28k and occupies 32k bytes.
 */


#define HCT_S1_LENGTH       ( sizeof( COMMAND_TABLE )  * MAX_SLOT_NUM )
#define HCT_S2_LENGTH       ( 0 )
#define HCT_S3_LENGTH       ( 0 )
#define HCT_S4_LENGTH       ( 0 )
#define HCT_FCQ_LENGTH      ( sizeof( HCT_FCQ_WBQ )   * MAX_FCQ )
#define HCT_WBQ_LENGTH      ( sizeof( HCT_FCQ_WBQ )   * WBQ_N * MAX_SLOT_NUM )


#define HCT_S0_BASE         ( HCT_DSRAM_BASE_ADDRESS  )
//#define HCT_S1_BASE         ( HCT_DSRAM_BASE_ADDRESS + HCT_S0_LENGTH )
#define HCT_S2_BASE         ( HCT_DSRAM_BASE_ADDRESS + HCT_S0_LENGTH + HCT_S1_LENGTH )
#define HCT_S3_BASE         ( HCT_DSRAM_BASE_ADDRESS + HCT_S0_LENGTH + HCT_S1_LENGTH + HCT_S2_LENGTH )
#define HCT_S4_BASE         ( HCT_DSRAM_BASE_ADDRESS + HCT_S0_LENGTH + HCT_S1_LENGTH + HCT_S2_LENGTH + HCT_S3_LENGTH )

#define HCT_FCQ_BASE            ( HCT_S1_BASE + HCT_S1_LENGTH )//( HCT_DSRAM_BASE_ADDRESS + 32 * 1024 )
#define HCT_WBQ_BASE            ( HCT_FCQ_BASE + HCT_FCQ_LENGTH )//( HCT_DSRAM_BASE_ADDRESS + 32 * 1024 + HCT_FCQ_LENGTH )
void L0_NVMEInitHCT(void);

#ifndef AF_ENABLE
U32 L0_NVMeHCTReadCH( PCB_MGR CbMgr );
#endif

U32 L0_NVMeHCTReadPRP(PCB_MGR CbMgr);

extern volatile HCT_CONTROL_REG    *g_pHCTControlReg;

#endif // _NVME_HCT_H
/********************FILE END*********************/

