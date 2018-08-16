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
Filename     :  COM_head.h
Version      :  Ver 1.0
Date         :
Author         :    AddisonSu

Description: Constants and macros definition for firmware header.

Modification History:
20120903    AddisonSu    001 first create
20140612    VictorZhang  002 Recreate for 3514 FW 1.060.0.0
*******************************************************************************/
#ifndef __HEAD_H__
#define __HEAD_H__
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_DMAE.h"
#include "HAL_Xtensa.h"

typedef struct GLB_REG_SOFTREST
{
    U32 bsRstMcu1   :1;
    U32 bsRstMcu1If :1;
    U32 bsRstMcu2   :1;
    U32 bsRstMcu2If :1;
    U32 bsRes    :25;
    U32 bsRstMcu0   :1;
    U32 bsRstMcu0If :1;
    U32 bsRstMcuIf  :1;
}GLB_REG_SOFTREST;

typedef struct GLB_REG_MCU
{
    U32 bsRMcu0StatVectorSel    :1;
    U32 bsMcu0RunStall          :1;
    U32 bsRMcu0OCDHaltOnReset   :1;
    U32 bsRes0                  :1;

    U32 bsRMcu1StatVectorSel    :1;
    U32 bsMCU1RunStall          :1;
    U32 bsRMcu1OCDHaltOnReset   :1;
    U32 bsRes1                  :1;

    U32 bsRMcu2StatVectorSel    :1;
    U32 bsMCU2RunStall          :1;
    U32 bsRMcu2OCDHaltOnReset   :1;
    U32 bsRes2                  :21;
}GLB_REG_MCU;

typedef struct HEAD_MAP_TABLE_
{
    U32 ulSramAddr;
    U32 ulDramAddr;
    U32 ulSize;
    U32 ulMcuId;
}HEAD_MAP_TABLE;

typedef struct __FW_FILE_HDR
{
    U32 m_bin_total_len; // The total length of the fw bin file
    U32 m_sec_des_off; //The file offset of the section description
}FW_FILE_HDR;

typedef struct __FW_SEC_DES_ELE_META
{
    U32 m_sec_offset; // the section offset to the base of the fw bin file
    U32 m_sec_len; // the section len
    U32 m_sec_phy_addr_off; // this section offset to the real start position of the fw in DRAM
       U32 m_b_duplite_section_in_bin;
}FW_SEC_DES_ELE_META;

#define STACK_SIZE (2 * 1024)

#define HEAD_SRAM_SEG_NUM 6

#define HEAD_OTFB_SEG_NUM 2
#define PG_SIZE_PER_PLN  (16 * 1024)
#define FW_VERSION_OFFSET 32
#define VERSION_TOTAL_LEN_DW 6

MCU0_DRAM_TEXT void HEAD_RstMcu12(U32);
MCU0_DRAM_TEXT void HEAD_StallMcu12(U32);
INLINE void HEAD_RelocationDramToSram(U32);
INLINE void HEAD_RelocationSramToDram(BOOL bToSleep);
MCU0_DRAM_TEXT void HEAD_Main(void);
#endif

/********************** FILE END ***************/

