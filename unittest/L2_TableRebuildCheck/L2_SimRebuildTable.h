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
  File Name     : L2_SimRebuildTable.c
  Version       : Ver 1.0
  Author        : henryluo
  Created       : 2015/02/28
  Description   : 
  Function List :
  History       :
  1.Date        : 2015/02/28
    Author      : henryluo
    Modification: Created file

*******************************************************************************/
#ifndef _H_SIM_REBUILD_SSD
#define _H_SIM_REBUILD_SSD
#ifdef SIM
#include "L2_PMTPage.h"

#define TEMP_TARGET_CNT 2
#define MAX_PENDING_LPN ((SUBSYSTEM_PU_NUM)*((NFCQ_DEPTH) + (NFCQ_DEPTH))*(LPN_PER_BUF))
#define PG_TYPE_FREE 0xFF
typedef struct _NFC_REBUILD_DBG_INFO
{
    //For rebuild PMT
    U32* m_pRedTS;
    PhysicalAddr* m_pNfcRebuildPMT;

    //For rebuild PMTI and PMTManager
    U32* m_pPMTPageTS[SUBSYSTEM_LUN_MAX];
    PhysicalAddr* m_pPMTAddr[SUBSYSTEM_LUN_MAX];

    //record CE_FIFO and CQ Entry pending write LPN
    U32* m_pPendingLPN;

    U16 m_SameVirBlk[SUBSYSTEM_LUN_MAX];
}NFC_REBUILD_DBG_INFO;

typedef enum _TABLE_REBUILD_SOURCE
{
    TABLE_REBUILD_DRAM,
    TABLE_REBUILD_FLASH,
}TABLE_REBUILD_SOURCE;

void SIM_RebuildSSD_Init();
void SIM_RebuildPMTManager_PMTI();
void SIM_Rebuild_DBGInfo();
void SIM_BackTableFromDram(void);

#endif // end #ifdef SIM

#endif