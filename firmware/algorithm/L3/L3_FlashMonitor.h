/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
* File Name    : L3_FlashMonitor.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_FLASHMONITOR_H
#define _L3_FLASHMONITOR_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "Disk_Config.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef struct _FM_USER_ITEM_
{
    U32 bsErsErrCnt     :16;
    U32 bsPrgErrCnt     :16;
    U32 bsReccErrCnt    :16;
    U32 bsUeccErrCnt    :16;
    U32 ulErsTime;
    U32 ulPrgTime;
    U32 ulReadTime;
}FM_USER_ITEM;

typedef struct _FM_INTR_ITEM_
{
    U32 bsPhyBlk        :16;
    U32 bsPhyPage       :16;
    U32 bsPln           :4;
    U32 bsCmdType       :8;
    U32 bsVthShiftRd    :8;
    U32 bsSLCMode       :8;
    U32 bsRsvd1         :4;
    U32 bsSlcVthRetry   :8;
    U32 bsMlcVthRetry   :8;
    U32 bsSlcVthRetryDft:8;
    U32 bsMlcVthRetryDft:8;
}FM_INTR_ITEM;

#define FM_USER_TOT_SZ (SUBSYSTEM_LUN_NUM * sizeof(FM_USER_ITEM))
#define FM_INTR_TOT_SZ (SUBSYSTEM_LUN_NUM * sizeof(FM_INTR_ITEM))

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
extern MCU12_VAR_ATTR FM_USER_ITEM *g_ptFMUserMgr;

void L3_FMAllocSRAM0(U32 *pFreeSram0Base);

void MCU2_DRAM_TEXT L3_FMIntrInit(void);
FM_USER_ITEM *L3_FMGetUsrItem(U8 ucTLun);
U32 MCU12_DRAM_TEXT L3_FMGetTotErsCnt(void);
U32 MCU12_DRAM_TEXT L3_FMGetTotPrgCnt(void);
void L3_FMUpdtUsrOpCnt(U8 ucTLun, U8 ucFCmdType, U8 ucPairPageCnt);
void MCU2_DRAM_TEXT L3_FMUpdtUsrFailCnt(U8 ucTLun, U8 ucFCmdType);
U16 L3_FMGetPhyBlk(U8 ucTLun);
U16 L3_FMGetPhyPage(U8 ucTLun);
U8 L3_FMGetPlnNum(U8 ucTLun);
U8 L3_FMGetCmdType(U8 ucTLun);
void L3_FMSetPhyBlk(U8 ucTLun, U16 usPhyBlk);
void L3_FMSetPhyPage(U8 ucTLun, U16 usPhyPage);
void L3_FMSetPlnNum(U8 ucTLun, U8 Pln);
void L3_FMSetCmdType(U8 ucTLun, U8 ucCmdType);
BOOL L3_FMGetSLCMode(U8 ucTLun);
void L3_FMSetSLCMode(U8 ucTLun, BOOL bSLCMode);

// For read retry
void MCU2_DRAM_TEXT L3_FMSetVthShiftRd(U8 ucTLun, U8 ucVth);
void MCU2_DRAM_TEXT L3_FMSetSlcVthRetry(U8 ucTLun, U8 ucVth);
void MCU2_DRAM_TEXT L3_FMSetSlcVthRetryDft(U8 ucTLun, U8 ucVth);
void MCU2_DRAM_TEXT L3_FMSetMlcVthRetry(U8 ucTLun, U8 ucVth);
void MCU2_DRAM_TEXT L3_FMSetMlcVthRetryDft(U8 ucTLun, U8 ucVth);
U8 MCU2_DRAM_TEXT L3_FMGetVthShiftRd(U8 ucTLun);
U8 MCU2_DRAM_TEXT L3_FMGetSlcVthRetry(U8 ucTLun);
U8 MCU2_DRAM_TEXT L3_FMGetSlcVthRetryDft(U8 ucTLun);
U8 MCU2_DRAM_TEXT L3_FMGetMlcVthRetry(U8 ucTLun);
U8 MCU2_DRAM_TEXT L3_FMGetMlcVthRetryDft(U8 ucTLun);
#endif
/*====================End of this head file===================================*/

