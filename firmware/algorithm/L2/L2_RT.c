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
File Name     : L2_RT.c
Version       : Initial version
Author        : henryluo
Created       : 2015/02/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2012/09/10
Author      : AwayWei
Modification: Created file

*******************************************************************************/
#include "HAL_Inc.h"
#include "L3_HostAdapter.h"
#include "L2_Defines.h"
#include "L2_Debug.h"
#include "L2_PMTManager.h"
#include "L2_StripeInfo.h"
#include "L2_GCManager.h"
#include "L2_Boot.h"
#include "L2_LLF.h"
#include "L2_TableBlock.h"
#include "L2_Thread.h"
#include "L1_GlobalInfo.h"
#include "L2_StaticWL.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "L2_RT.h"
#include "L2_TableBBT.h"
#include "L2_Interface.h"
#include "L2_FCMDQ.h"
#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif

GLOBAL  RT *pRT;

GLOBAL  PhysicalAddr RootTableAddr[RT_PAGE_COUNT];
GLOBAL  PhysicalAddr RootTableRecycleAddr;

GLOBAL  L2_RT_SAVE_STATUS gRTSaveStatus;
GLOBAL  BOOL bRTSaveAgain;
GLOBAL  RT_MANAGEMENT *pRTManagement;
GLOBAL  U16 usRTLoadPagePos;

extern GLOBAL  U16 L2RTStartTempBufID;
extern GLOBAL  U16 L2AT0StartTempBufID;
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;

extern void L2_FtlWriteLocal(PhysicalAddr* pAddr, U32* pPage, U32* pSpare, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, XOR_PARAM *pXorParam);
extern void L2_FtlReadLocal(U32* pBuffer, PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode);
extern void L2_FtlEraseBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 BlockInCE, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, BOOL bNeedL3ErrorHandle);
extern void FW_DbgUpdateStaticInfo(void);


void MCU1_DRAM_TEXT L2_RT_Init_Clear_All(void)
{
    HAL_DMAESetValue((U32)pRT, COM_MemSize16DWAlign(RT_SIZE), 0);

    return;
}

/* reset (1)RootTableAddr, (2)Current AT0/AT1 target infor, (3)all AT0 table location, 
(4) each AT0 block Addr and erase flag, (5) each AT1 block Addr and dirtyCnt/free flag.
(6) each Trace block Addr */
void MCU1_DRAM_TEXT L2_RT_Init(void)
{
    U8  ucLunInSuperPu;
    U8  ucSuperPuNum;
    U16 usPageIndex;

    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
    {
        RootTableAddr[usPageIndex].m_PPN = INVALID_8F;
    }

    RootTableRecycleAddr.m_PPN = INVALID_8F;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = INVALID_4F;
        }
        pRT->m_RT[ucSuperPuNum].CurAT0PPO = INVALID_4F;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = INVALID_4F;
        }
        pRT->m_RT[ucSuperPuNum].CurAT1PPO = INVALID_4F;

        pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp = 0;

        for (usPageIndex = 0; usPageIndex < PMTI_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].PMTIPPO[usPageIndex] = INVALID_4F;
        }

#ifndef LCT_VALID_REMOVED
        for (usPageIndex = 0; usPageIndex < VBMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].VBMTPPO[usPageIndex] = INVALID_4F;
        }
#endif

        for (usPageIndex = 0; usPageIndex < VBT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].VBTPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < PBIT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].PBITPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < DPBM_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].DPBMPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < RPMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].RPMTPPO[usPageIndex] = INVALID_4F;
        }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        for (usPageIndex = 0; usPageIndex < TLCW_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].TLCWPPO[usPageIndex] = INVALID_4F;
        }
#endif

        for (usPageIndex = 0; usPageIndex < AT0_BLOCK_COUNT; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][usPageIndex] = INVALID_4F;
                pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usPageIndex] = FALSE;
            }
        }

        for (usPageIndex = 0; usPageIndex < AT1_BLOCK_COUNT; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].AT1BlockDirtyCnt[usPageIndex] = 0;
            pRT->m_RT[ucSuperPuNum].AT1BlockFreeFlag[usPageIndex] = FALSE;
        }

        for (usPageIndex = 0; usPageIndex < TRACE_BLOCK_COUNT; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
        }
    }

    return;
}

/* reset (1)Current AT0/AT1 target infor, (2)all AT0 table location, 
(3) each AT0 block Addr and erase flag, (4) each AT1 block Addr and dirtyCnt/free flag.
(5) each Trace block Addr */
void MCU1_DRAM_TEXT L2_RT_Init_TableLocation(void)
{
    U8  ucLunInSuperPu;
    U8  ucSuperPuNum;
    U16 usPageIndex;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = INVALID_4F;
        }
        pRT->m_RT[ucSuperPuNum].CurAT0PPO = INVALID_4F;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = INVALID_4F;
        }
        pRT->m_RT[ucSuperPuNum].CurAT1PPO = INVALID_4F;

        pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp = 0;

        for (usPageIndex = 0; usPageIndex < PMTI_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].PMTIPPO[usPageIndex] = INVALID_4F;
        }

#ifndef LCT_VALID_REMOVED
        for (usPageIndex = 0; usPageIndex < VBMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].VBMTPPO[usPageIndex] = INVALID_4F;
        }
#endif

        for (usPageIndex = 0; usPageIndex < VBT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].VBTPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < PBIT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].PBITPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < DPBM_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].DPBMPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < RPMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].RPMTPPO[usPageIndex] = INVALID_4F;
        }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        for (usPageIndex = 0; usPageIndex < TLCW_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].TLCWPPO[usPageIndex] = INVALID_4F;
        }
#endif
        for (usPageIndex = 0; usPageIndex < AT0_BLOCK_COUNT; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][usPageIndex] = INVALID_4F;
                pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usPageIndex] = FALSE;
            }
        }

        for (usPageIndex = 0; usPageIndex < AT1_BLOCK_COUNT; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].AT1BlockDirtyCnt[usPageIndex] = 0;
            pRT->m_RT[ucSuperPuNum].AT1BlockFreeFlag[usPageIndex] = FALSE;
        }

        for (usPageIndex = 0; usPageIndex < TRACE_BLOCK_COUNT; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
        }
    }

    return;
}

/* reset (1)Current AT0/AT1 target infor, (2)all AT0 table location, (3) each AT0 block erase flag, 
(4) each AT1 block dirtyCnt/free flag. P.S. Don't reset AT0/AT1/Trace block Addr */
void MCU1_DRAM_TEXT L2_RT_Init_AT0AT1Infor(void)
{
    U8  ucLunInSuperPu;
    U8  ucSuperPuNum;
    U16 usPageIndex;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = INVALID_4F;
        }
        pRT->m_RT[ucSuperPuNum].CurAT0PPO = INVALID_4F;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = INVALID_4F;
        }
        pRT->m_RT[ucSuperPuNum].CurAT1PPO = INVALID_4F;

        //pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp = 0;

        for (usPageIndex = 0; usPageIndex < PMTI_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].PMTIPPO[usPageIndex] = INVALID_4F;
        }

#ifndef LCT_VALID_REMOVED
        for (usPageIndex = 0; usPageIndex < VBMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].VBMTPPO[usPageIndex] = INVALID_4F;
        }
#endif

        for (usPageIndex = 0; usPageIndex < VBT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].VBTPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < PBIT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].PBITPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < DPBM_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].DPBMPPO[usPageIndex] = INVALID_4F;
        }

        for (usPageIndex = 0; usPageIndex < RPMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].RPMTPPO[usPageIndex] = INVALID_4F;
        }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        for (usPageIndex = 0; usPageIndex < TLCW_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][usPageIndex] = INVALID_4F;
            }
            pRT->m_RT[ucSuperPuNum].TLCWPPO[usPageIndex] = INVALID_4F;
        }
#endif
        for (usPageIndex = 0; usPageIndex < AT0_BLOCK_COUNT; usPageIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usPageIndex] = FALSE;
            }
        }

        for (usPageIndex = 0; usPageIndex < AT1_BLOCK_COUNT; usPageIndex++)
        {
            pRT->m_RT[ucSuperPuNum].AT1BlockDirtyCnt[usPageIndex] = 0;
            pRT->m_RT[ucSuperPuNum].AT1BlockFreeFlag[usPageIndex] = FALSE;
        }
    }

    return;
}


void MCU1_DRAM_TEXT L2_RTManagement_Init(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    RED* pRed;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRTManagement->LoadStatus[ucSuperPuNum][ucLunInSuperPu] = L2_RT_LOAD_START;
            pRTManagement->CheckStatus[ucSuperPuNum][ucLunInSuperPu] = L2_RT_CHECK_START;
            pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;

            pRed = &(pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]);

            //Fix me : SuperPage Lun 1:3 causes DMAESetValue() fail issue.
            //HAL_DMAESetValue((U32)pRed, (RED_SW_SZ_DW * DWORD_SIZE), 0);
            COM_MemSet((U32 *)pRed, RED_SW_SZ_DW, 0);
        }
    }

    gRTSaveStatus = L2_RT_SAVE_START;

    return;
}

void MCU1_DRAM_TEXT L2_RT_Rebuild_MarkTable(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;
    U16 usBlockPos;
    U16 usBlockIndex;
    U32 usTableBlockEnd;
    U32 usDataBlockStart;

    usBlockPos = (GLOBAL_BLOCK_COUNT + BBT_BLOCK_COUNT);

    /* mark RT block */
    usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockPos, TRUE);
    pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu] = usBlock;
    L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);

    usBlockPos += RT_BLOCK_COUNT;

    /* mark AT0 block */
    for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + AT0_BLOCK_COUNT); usBlockIndex++)
    {
        usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
        pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(usBlockIndex - usBlockPos)] = usBlock;
        L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
    }

    pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = INVALID_4F;
    pRT->m_RT[ucSuperPuNum].CurAT0PPO = INVALID_4F;

    usBlockPos += AT0_BLOCK_COUNT;

    /* mark AT1 block */
    for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + AT1_BLOCK_COUNT); usBlockIndex++)
    {
        usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
        pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][(usBlockIndex - usBlockPos)] = usBlock;
        L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
    }

    pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = INVALID_4F;
    pRT->m_RT[ucSuperPuNum].CurAT1PPO = INVALID_4F;

    usBlockPos += AT1_BLOCK_COUNT;

    /* mark Trace block */
    for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + TRACE_BLOCK_COUNT); usBlockIndex++)
    {
        usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
        pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][(usBlockIndex - usBlockPos)] = usBlock;
        L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
    }

    g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][TRACE_BLOCK_COUNT - 1];

    usBlockPos += TRACE_BLOCK_COUNT;

    /* mark Rsvd block */
    for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + RSVD_BLOCK_COUNT); usBlockIndex++)
    {
        usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
        L2_PBIT_Set_Reserve(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
    }

    usBlockPos += RSVD_BLOCK_COUNT;

    /* mark data block start */
    g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu] = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockPos, TRUE);
    L2_BbtGetRootPointer(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), &usTableBlockEnd, &usDataBlockStart);
    if (g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu] > usDataBlockStart)
    {
        g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu] = usDataBlockStart;
    }
    return;
}

void MCU1_DRAM_TEXT L2_RT_ResetSaveStatus(void)
{
    gRTSaveStatus = L2_RT_SAVE_START;

    return;
}

void MCU1_DRAM_TEXT L2_RT_ResetLoadStatus(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRTManagement->LoadStatus[ucSuperPuNum][ucLunInSuperPu] = L2_RT_LOAD_START;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_RT_ResetCheckStatus(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRTManagement->CheckStatus[ucSuperPuNum][ucLunInSuperPu] = L2_RT_CHECK_START;
        }
    }

    return;
}

U8 MCU1_DRAM_TEXT L2_RT_GetAT1BlockSNFromPBN(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usBlock)
{
    U8 ucBlockSN;
    U8 ucBlockIndex;

    ucBlockSN = INVALID_2F;

    for (ucBlockIndex = 0; ucBlockIndex < AT1_BLOCK_COUNT; ucBlockIndex++)
    {
        if (usBlock == pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][ucBlockIndex])
        {
            ucBlockSN = ucBlockIndex;
            break;
        }
    }

    return ucBlockSN;
}

U16 MCU1_DRAM_TEXT L2_RT_GetAT1BlockPBNFromSN(U8 ucSuperPuNum, U8 ucLunInSuperPu, U8 ucBlockSN)
{
    U16 usBlock;

    usBlock = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][ucBlockSN];

    return usBlock;
}


void MCU1_DRAM_TEXT L2_RTPrepareSysInfoAT1(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    U8 ucCurBlkSN;
    U8 ucBlockIndex;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp = L2_GetTimeStampInPu(ucSuperPuNum);

            ucCurBlkSN = (U8)g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_CurBlkSN;

            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = L2_RT_GetAT1BlockPBNFromSN(ucSuperPuNum, ucLunInSuperPu, ucCurBlkSN);
            pRT->m_RT[ucSuperPuNum].CurAT1PPO = g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_CurPPO;

            for (ucBlockIndex = 0; ucBlockIndex < AT1_BLOCK_COUNT; ucBlockIndex++)
            {
                pRT->m_RT[ucSuperPuNum].AT1BlockDirtyCnt[ucBlockIndex] = g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_PMTBlkInfo[ucBlockIndex].m_DirtyCnt;
                pRT->m_RT[ucSuperPuNum].AT1BlockFreeFlag[ucBlockIndex] = g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_PMTBlkInfo[ucBlockIndex].m_bFree;
            }
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_RTPrepareSysInfoFlashMonitor(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    FM_USER_ITEM *ptFlashMonitor;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ptFlashMonitor = L3_FMGetUsrItem(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu));
            pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsErsErrCnt = ptFlashMonitor->bsErsErrCnt;
            pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsPrgErrCnt = ptFlashMonitor->bsPrgErrCnt;
            pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsReccErrCnt = ptFlashMonitor->bsReccErrCnt;
            pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsUeccErrCnt = ptFlashMonitor->bsUeccErrCnt;
            pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.ulErsTime = ptFlashMonitor->ulErsTime;
            pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.ulPrgTime = ptFlashMonitor->ulPrgTime;
            pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.ulReadTime = ptFlashMonitor->ulReadTime;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_RTPrepareSysInfoTrace(void)
{
    COM_MemCpy((U32 *)&(pRT->m_TraceBlockManager), (U32 *)&g_TraceManagement, sizeof(TRACE_BLOCK_MANAGEMENT) / sizeof(U32));
    return;
}

void MCU1_DRAM_TEXT L2_RTPrepareSysInfoGlobalInfo(void)
{
    FW_DbgUpdateStaticInfo();

    COM_MemCpy((U32 *)&(pRT->m_HostInfo), (U32 *)g_pSubSystemHostInfoPage, sizeof(HOST_INFO_PAGE) / sizeof(U32));
    COM_MemCpy((U32 *)&(pRT->m_DevParam), (U32 *)g_pSubSystemDevParamPage, sizeof(DEVICE_PARAM_PAGE) / sizeof(U32));

    return;
}

void MCU1_DRAM_TEXT L2_RTPrepareSysInfo(void)
{
    L2_RTPrepareSysInfoAT1();

    L2_RTPrepareSysInfoFlashMonitor();

    L2_RTPrepareSysInfoTrace();

    L2_RTPrepareSysInfoGlobalInfo();

    return;
}

void MCU1_DRAM_TEXT L2_RTResumeSysInfoAT1(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    U8 ucBlockIndex;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            /* Resume max TS */
            g_PuInfo[ucSuperPuNum]->m_TimeStamp = pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp;

            g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_CurPPO = pRT->m_RT[ucSuperPuNum].CurAT1PPO;

            for (ucBlockIndex = 0; ucBlockIndex < AT1_BLOCK_COUNT; ucBlockIndex++)
            {
                if (pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][ucBlockIndex] == pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu])
                {
                    g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_CurBlkSN = ucBlockIndex;
                }

                g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_PMTBlkInfo[ucBlockIndex].m_DirtyCnt = pRT->m_RT[ucSuperPuNum].AT1BlockDirtyCnt[ucBlockIndex];
                g_PMTManager->m_PMTBlkManager[ucSuperPuNum].m_PMTBlkInfo[ucBlockIndex].m_bFree = pRT->m_RT[ucSuperPuNum].AT1BlockFreeFlag[ucBlockIndex];
            }
        }
    }
    return;
}

void MCU1_DRAM_TEXT L2_RTResumeSysInfoFlashMonitor(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    FM_USER_ITEM *ptFlashMonitor;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ptFlashMonitor = L3_FMGetUsrItem(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu));
            ptFlashMonitor->bsErsErrCnt = pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsErsErrCnt;
            ptFlashMonitor->bsPrgErrCnt = pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsPrgErrCnt;
            ptFlashMonitor->bsReccErrCnt = pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsReccErrCnt;
            ptFlashMonitor->bsUeccErrCnt = pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.bsUeccErrCnt;
            ptFlashMonitor->ulErsTime = pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.ulErsTime;
            ptFlashMonitor->ulPrgTime = pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.ulPrgTime;
            ptFlashMonitor->ulReadTime = pRT->m_RT[ucSuperPuNum].m_FlashMonitorItem.ulReadTime;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_RTResumeSysInfoTrace()
{
    COM_MemCpy((U32 *)&g_TraceManagement, (U32 *)&(pRT->m_TraceBlockManager), sizeof(TRACE_BLOCK_MANAGEMENT) / sizeof(U32));

    return;
}

void MCU1_DRAM_TEXT L2_RTResumeSysInfoGlobalInfo(void)
{
    COM_MemCpy((U32 *)g_pSubSystemHostInfoPage, (U32 *)&(pRT->m_HostInfo), sizeof(HOST_INFO_PAGE) / sizeof(U32));
    COM_MemCpy((U32 *)g_pSubSystemDevParamPage, (U32 *)&(pRT->m_DevParam), sizeof(DEVICE_PARAM_PAGE) / sizeof(U32));
    return;
}

void MCU1_DRAM_TEXT L2_RTResumeSysInfo(void)
{
    L2_RTResumeSysInfoAT1();

    L2_RTResumeSysInfoFlashMonitor();

    L2_RTResumeSysInfoTrace();

    L2_RTResumeSysInfoGlobalInfo();

    return;
}

U16 MCU1_DRAM_TEXT L2_RT_FindNextGoodBlock(U8 ucLunNum, U16 usCurBlockPos, BOOL bHead)
{
    U16 PhyBlockAddr;
    BOOL bBadBlk;
    S16 sOP;
    U16 usPhyBlkAddrBoundary;

    if (TRUE == bHead)
    {
        PhyBlockAddr = BBT_BLOCK_ADDR_BASE;
        usPhyBlkAddrBoundary = (BLK_PER_PLN + RSV_BLK_PER_PLN - 1);
        sOP = 1;
    }
    else
    {
        PhyBlockAddr = (BLK_PER_PLN + RSV_BLK_PER_PLN - 1);
        usPhyBlkAddrBoundary = BBT_BLOCK_ADDR_BASE;
        sOP = -1;
    }

    for (PhyBlockAddr = usCurBlockPos + sOP; PhyBlockAddr != usPhyBlkAddrBoundary; PhyBlockAddr += sOP)
    {
        //Search none bad block from BBT
        bBadBlk = L2_BbtIsGBbtBadBlock(ucLunNum, PhyBlockAddr);
        if (TRUE == bBadBlk)
        {
            continue;
        }

        return PhyBlockAddr;
    }

    return  INVALID_4F;
}

U16 MCU1_DRAM_TEXT L2_RT_Search_BlockBBT(U8 ucLunNum, U16 usNoneBadPos, BOOL bHead)
{
    U16 usNoneBadCnt;
    U16 PhyBlockAddr;
    U16 usPhyBlkAddrBoundary;
    BOOL bBadBlk;
    S16 sOP;

    if (TRUE == bHead)
    {
        PhyBlockAddr = BBT_BLOCK_ADDR_BASE;
        usPhyBlkAddrBoundary = (BLK_PER_PLN + RSV_BLK_PER_PLN - TLC_BLK_CNT);
        sOP = 1;
    }
    else
    {
        PhyBlockAddr = (BLK_PER_PLN + RSV_BLK_PER_PLN - 1);
        usPhyBlkAddrBoundary = BBT_BLOCK_ADDR_BASE;
        sOP = -1;
    }

    usNoneBadCnt = 0;

    for (PhyBlockAddr; PhyBlockAddr != usPhyBlkAddrBoundary; PhyBlockAddr += sOP)
    {
        //Search none bad block from BBT
        bBadBlk = L2_BbtIsGBbtBadBlock(ucLunNum, PhyBlockAddr);
        if (TRUE == bBadBlk)
        {
            continue;
        }

        usNoneBadCnt++;
        if (usNoneBadCnt == usNoneBadPos)
        {
            return PhyBlockAddr;
        }
    }

    return INVALID_4F;
}

LOCAL MCU12_VAR_ATTR U8 ucRTLoadNum;
extern void MCU1_DRAM_TEXT L2_TB_Rebuild_TableUpdateMaxTS(U8 ucSuperPuNum, RED *pRed);
U32 MCU1_DRAM_TEXT L2_RT_LoadRT(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U32 ulRet;
    U32 ulBufferAddr;
    U8  ucFlashStatus;
    L2_RT_LOAD_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };

    ulRet = L2_RT_WAIT;
    pStatus = &(pRTManagement->LoadStatus[ucSuperPuNum][ucLunInSuperPu]);

    switch (*pStatus)
    {
    case L2_RT_LOAD_START:
    {
        /* Init global values */
        if (ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu == 0)
        {
            U8 usPageIndex;

            for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
            {
                RootTableAddr[usPageIndex].m_PPN = INVALID_8F;
            }

            usRTLoadPagePos = 0;
            ucRTLoadNum = 0;
        }

        pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;
        pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu] = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, RT_BLOCK_ADDR_BASE, TRUE); //@@mavishuang: please do not use constant 2);

        *pStatus = L2_RT_LOAD_SEARCH_BLOCK;
    }
        break;

    case L2_RT_LOAD_SEARCH_BLOCK:
    {
        /* send FlashReq to read each Block first page redundant */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            FlashAddr.m_PUSer = ucSuperPuNum;
            FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
            FlashAddr.m_BlockInPU = pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu];
            FlashAddr.m_PageInBlock = 0;
            FlashAddr.m_LPNInPage = 0;

            L2_TableLoadSpare(&FlashAddr, &(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)&(pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]));
            *pStatus = L2_RT_LOAD_CHECK_BLOCK;
        }
    }
        break;

    case L2_RT_LOAD_CHECK_BLOCK:
    {
        /* check RT located PU and Block */
        ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            if (PAGE_TYPE_RT == pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu].m_RedComm.bcPageType)
            {
                if (INVALID_8F != RootTableAddr[0].m_PPN)
                {
                    /* more than 1 PU saved RT: Rebuild */
                    *pStatus = L2_RT_LOAD_FAIL;
                }
                else
                {
                    /* find RT located PU and Block */
                    U8 usPageIndex;

                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PUSer = ucSuperPuNum;
                        RootTableAddr[usPageIndex].m_OffsetInSuperPage = ucLunInSuperPu;
                        RootTableAddr[usPageIndex].m_BlockInPU = pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu];
                    }

                    *pStatus = L2_RT_LOAD_SEARCH_PAGE;
                }
            }
            else
            {
                /* load RT PageType wrong: Rebuild */
                *pStatus = L2_RT_LOAD_FAIL;
            }
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            /* RT not in current PU: finish in current PU */
            *pStatus = L2_RT_LOAD_SUCCESS;
        }
        else
        {
            /* read RT block 1st page UECC: Rebuild RT */
            *pStatus = L2_RT_LOAD_FAIL;
        }
    }
        break;

    case L2_RT_LOAD_SEARCH_PAGE:
    {
        /* send FlashReq to read each RT Page */
        if (PG_PER_SLC_BLK <= usRTLoadPagePos)
        {
            DBG_Printf("L2_RT_LoadRT search Page Index Error PU 0x%x LUN 0x%x Block 0x%x\n", RootTableAddr[0].m_PUSer, RootTableAddr[0].m_OffsetInSuperPage, RootTableAddr[0].m_BlockInPU);
            DBG_Getch();
        }

        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            FlashAddr.m_PUSer = ucSuperPuNum;
            FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
            FlashAddr.m_BlockInPU = RootTableAddr[0].m_BlockInPU;
            FlashAddr.m_PageInBlock = usRTLoadPagePos;
            FlashAddr.m_LPNInPage = 0;

            pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

            ulBufferAddr = COM_GetMemAddrByBufferID((L2RTStartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
            L2_FtlReadLocal((U32 *)ulBufferAddr, &FlashAddr, &(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                (U32 *)&(pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]), LPN_PER_BUF, 0, TRUE, TRUE);

            *pStatus = L2_RT_LOAD_CHECK_PAGE;
        }
    }
        break;

    case L2_RT_LOAD_CHECK_PAGE:
    {
        /* check LB located Page */
        ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            if (PAGE_TYPE_RT == pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu].m_RedComm.bcPageType)
            {
                if ((PG_PER_SLC_BLK - 1) == usRTLoadPagePos)
                {
                    /* last page of current block */
                    U8 usPageIndex;

                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PageInBlock = usRTLoadPagePos - (RT_PAGE_COUNT - usPageIndex - 1);
                        RootTableAddr[usPageIndex].m_LPNInPage = 0;
                    }

                    *pStatus = L2_RT_LOAD_READ_RT;
                }
                else
                {
                    usRTLoadPagePos++;
                    *pStatus = L2_RT_LOAD_SEARCH_PAGE;
                }
            }
            else
            {
                /* Wrong Page Type: Rebuild RT */
                *pStatus = L2_RT_LOAD_FAIL;
            }
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            U8 usPageIndex;

            for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
            {
                RootTableAddr[usPageIndex].m_PageInBlock = usRTLoadPagePos - (RT_PAGE_COUNT - usPageIndex);
                RootTableAddr[usPageIndex].m_LPNInPage = 0;
            }
            *pStatus = L2_RT_LOAD_READ_RT;
        }
        else
        {
            /* Load RT Page UECC: Rebuild RT */
            *pStatus = L2_RT_LOAD_FAIL;
        }
    }
        break;

    case L2_RT_LOAD_READ_RT:
    {
        /* send FlashReq to read latest RT to DRAM */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

            FlashAddr.m_PUSer = RootTableAddr[ucRTLoadNum].m_PUSer;
            FlashAddr.m_OffsetInSuperPage = RootTableAddr[ucRTLoadNum].m_OffsetInSuperPage;
            FlashAddr.m_BlockInPU = RootTableAddr[ucRTLoadNum].m_BlockInPU;
            FlashAddr.m_PageInBlock = RootTableAddr[ucRTLoadNum].m_PageInBlock;
            FlashAddr.m_LPNInPage = 0;

            ulBufferAddr = COM_GetMemAddrByBufferID((L2RTStartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
            L2_FtlReadLocal((U32 *)ulBufferAddr, &FlashAddr, &(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                (U32 *)&(pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]), LPN_PER_BUF, 0, TRUE, TRUE);

            *pStatus = L2_RT_LOAD_WAIT_FINISH;
        }
    }
        break;

    case L2_RT_LOAD_WAIT_FINISH:
    {
        /* wait flash idle and copy data to pRT */
        ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            U32 ulDataAddr;

            ulBufferAddr = COM_GetMemAddrByBufferID((L2RTStartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
            ulDataAddr = (U32)pRT + ucRTLoadNum*BUF_SIZE;
            COM_MemCpy((U32 *)ulDataAddr, (U32 *)ulBufferAddr, BUF_SIZE / sizeof(U32));
            L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, &pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]);

            ucRTLoadNum++;
            if (ucRTLoadNum >= RT_PAGE_COUNT)
                *pStatus = L2_RT_LOAD_SUCCESS;
            else
                *pStatus = L2_RT_LOAD_READ_RT;
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            /*  Load RT Page 2nd time EMPTY_PG: must be function error */
            DBG_Printf("L2_RT_LoadRT PU 0x%x LUN 0x%x WAIT_FINISH EMPTY_PG\n", ucSuperPuNum, ucLunInSuperPu);
            DBG_Getch();
            *pStatus = L2_RT_LOAD_FAIL;
        }
        else
        {
            /* Load RT Page 2nd time UECC: Rebuild RT */
            *pStatus = L2_RT_LOAD_FAIL;
        }
    }
        break;

    case L2_RT_LOAD_FAIL:
    {
        ulRet = L2_RT_FAIL;
    }
        break;

    case L2_RT_LOAD_SUCCESS:
    {
        ulRet = L2_RT_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_RT_LoadRT invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_RT_CheckRT(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U32 ulRet;
    U32 ulBufferAddr;
    U8  ucFlashStatus;
    RED *pRT_Red;
    L2_RT_CHECK_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };
    PhysicalAddr InvalidFlashAddr;

    InvalidFlashAddr.m_PPN = INVALID_8F;
    ulRet = L2_RT_WAIT;
    pStatus = &(pRTManagement->CheckStatus[ucSuperPuNum][ucLunInSuperPu]);

    switch (*pStatus)
    {
    case L2_RT_CHECK_START:
    {
        /* check RT found or not */
        if (InvalidFlashAddr.m_PUSer == RootTableAddr[0].m_PUSer)
        {
            *pStatus = L2_RT_CHECK_FAIL;
            break;
        }

        /* check Red common part */
        if ((ucSuperPuNum == RootTableAddr[0].m_PUSer) && (ucLunInSuperPu == RootTableAddr[0].m_OffsetInSuperPage))
        {
            pRT_Red = &(pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]);

            if ((PAGE_TYPE_RT != pRT_Red->m_RedComm.bcPageType)/* || (TRUE != pRT_Red->bPowerCycle)*/)
            {
                /* RT Redundant wrong, abnormal pwoer cycling, rebuild all table */
                g_TraceManagement.m_NeedRebuild = TRUE;
                *pStatus = L2_RT_CHECK_FAIL;
                break;
            }
        }

        /* check RT AT0 Table Physical Block/Page info whether valid */
        if ((INVALID_4F == pRT->m_RT[ucSuperPuNum].VBTBlock[0][0])
#ifndef LCT_VALID_REMOVED
            || (INVALID_4F == pRT->m_RT[ucSuperPuNum].VBMTBlock[0][0])
#endif
            || (INVALID_4F == pRT->m_RT[ucSuperPuNum].PMTIBlock[0][0])
            || (INVALID_4F == pRT->m_RT[ucSuperPuNum].PBITBlock[0][0])
            || (INVALID_4F == pRT->m_RT[ucSuperPuNum].DPBMBlock[0][0])
            || (INVALID_4F == pRT->m_RT[ucSuperPuNum].RPMTBlock[0][0]))
        {
            *pStatus = L2_RT_CHECK_FAIL;
            break;
        }

        /* Init global values */
        pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;
        *pStatus = L2_RT_CHECK_READ_AT0_PPO;
    }
        break;

    case L2_RT_CHECK_READ_AT0_PPO:
    {
        /* send FlashReq to read AT0 Block Current PPO page */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            if (((pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] < L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, AT0_BLOCK_ADDR_BASE, TRUE)) //@@mavishuang: please do not use constant  3))
                || (pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] > L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, (AT1_BLOCK_ADDR_BASE - 1), TRUE))) //@@mavishuang: please do not use constant 4))) 
                || (pRT->m_RT[ucSuperPuNum].CurAT0PPO >= PG_PER_SLC_BLK))
            {
                /* RT CurAT0Block or CurAT0PPO wrong, rebuild all table */
                *pStatus = L2_RT_CHECK_FAIL;
            }
            else
            {
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu];
                FlashAddr.m_PageInBlock = pRT->m_RT[ucSuperPuNum].CurAT0PPO;
                FlashAddr.m_LPNInPage = 0;

                ulBufferAddr = COM_GetMemAddrByBufferID((L2AT0StartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
                L2_FtlReadLocal((U32 *)ulBufferAddr, &FlashAddr, &(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                    NULL, LPN_PER_BUF, 0, TRUE, TRUE);

                *pStatus = L2_RT_CHECK_WAIT_AT0_PPO;
            }
        }
    }
        break;

    case L2_RT_CHECK_WAIT_AT0_PPO:
    {
        /* read AT0 Current Block and PPO should return EMPTY_PG */
        ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            break;
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            *pStatus = L2_RT_CHECK_READ_AT1_PPO;
        }
        else
        {
            /* read AT0 Current Block and PPO UECC or SUCCESS: Rebuild RT */
            *pStatus = L2_RT_CHECK_FAIL;
        }
    }
        break;

    case L2_RT_CHECK_READ_AT1_PPO:
    {
        /* send FlashReq to read AT1 Block Current PPO page */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            if (((pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] < L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, AT1_BLOCK_ADDR_BASE, TRUE)) //@@mavishuang: please do not use constant  5))
                || (pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] > L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, (AT1_BLOCK_ADDR_BASE + AT1_BLOCK_COUNT - 1), TRUE))) //@@mavishuang: please do not use constant (5 + AT1_BLOCK_COUNT - 1)))) 
                || (pRT->m_RT[ucSuperPuNum].CurAT1PPO >= PG_PER_SLC_BLK))
            {
                /* RT CurAT1Block or CurAT1PPO wrong, rebuild all table */
                *pStatus = L2_RT_CHECK_FAIL;
            }
            else
            {
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu];
                FlashAddr.m_PageInBlock = pRT->m_RT[ucSuperPuNum].CurAT1PPO;
                FlashAddr.m_LPNInPage = 0;

                ulBufferAddr = COM_GetMemAddrByBufferID((L2AT0StartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
                L2_FtlReadLocal((U32 *)ulBufferAddr, &FlashAddr, &(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                    NULL, LPN_PER_BUF, 0, TRUE, TRUE);

                *pStatus = L2_RT_CHECK_WAIT_AT1_PPO;
            }
        }
    }
        break;

    case L2_RT_CHECK_WAIT_AT1_PPO:
    {
        /* read AT1 Current Block and PPO should return EMPTY_PG */
        ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            break;
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            *pStatus = L2_RT_CHECK_SUCCESS;
        }
        else
        {
            /* read AT1 Current Block and PPO UECC or SUCCESS: Rebuild RT */
            *pStatus = L2_RT_CHECK_FAIL;
        }
    }
        break;


    case L2_RT_CHECK_FAIL:
    {
        ulRet = L2_RT_FAIL;
    }
        break;

    case L2_RT_CHECK_SUCCESS:
    {
        ulRet = L2_RT_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_RT_CheckRT invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

void MCU1_DRAM_TEXT L2_RT_Save_Allocate_NextFreeBlock(void)
{
    U8  ucSuperPuNum;
    U8  ucLunInSuperPu;
    U16 ucRTBlk;
    U16 usPageIndex;

    ucSuperPuNum = RootTableAddr[0].m_PUSer;
    ucLunInSuperPu = RootTableAddr[0].m_OffsetInSuperPage;

    // Update ucLunInSuperPu and ucSuperPuNum
    ucLunInSuperPu++;
    if (ucLunInSuperPu >= LUN_NUM_PER_SUPERPU)
    {
        ucLunInSuperPu = 0;
        ucSuperPuNum++;
        if (ucSuperPuNum >= SUBSYSTEM_SUPERPU_NUM)
        {
            ucSuperPuNum = 0;
        }
    }

    ucRTBlk = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, RT_BLOCK_ADDR_BASE, TRUE); //@@mavishuang: please do not use constant 2);

    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
    {
        RootTableAddr[usPageIndex].m_PUSer = ucSuperPuNum;
        RootTableAddr[usPageIndex].m_OffsetInSuperPage = ucLunInSuperPu;
        RootTableAddr[usPageIndex].m_BlockInPU = ucRTBlk;
        RootTableAddr[usPageIndex].m_PageInBlock = usPageIndex;
        RootTableAddr[usPageIndex].m_LPNInPage = 0;
    }

    return;
}

void MCU1_DRAM_TEXT L2_RT_Save_UpdateFlashAddr(void)
{
    if (INVALID_8F == RootTableAddr[0].m_PPN)
    {
        U16 ucRTBlk = L2_RT_Search_BlockBBT(0, RT_BLOCK_ADDR_BASE, TRUE);
        U16 usPageIndex;

        for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
        {
            /* save first RT */
            RootTableAddr[usPageIndex].m_PUSer = 0;
            RootTableAddr[usPageIndex].m_OffsetInSuperPage = 0; // Lun
            RootTableAddr[usPageIndex].m_BlockInPU = ucRTBlk;
            RootTableAddr[usPageIndex].m_PageInBlock = usPageIndex;
        }

        RootTableRecycleAddr.m_PPN = INVALID_8F;
    }
    else
    {
        if ((RootTableAddr[RT_PAGE_COUNT - 1].m_PageInBlock + RT_PAGE_COUNT) <= (PG_PER_SLC_BLK - 1))
        {
            U8 ucPageIndex;

            for (ucPageIndex = 0; ucPageIndex < RT_PAGE_COUNT; ucPageIndex++)
            {
                RootTableAddr[ucPageIndex].m_PageInBlock += RT_PAGE_COUNT;
            }

            RootTableRecycleAddr.m_PPN = INVALID_8F;
        }
        else
        {

            RootTableRecycleAddr.m_PPN = RootTableAddr[0].m_PPN;

            L2_RT_Save_Allocate_NextFreeBlock();
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_RT_Save_WritePage(BOOL bPowerCycle, U8 ucPageIndex)
{
    U8  ucLoop;
    U8  ucSuperPuNum;
    U8  ucLunInSuperPu;
    U32 ulBufferAddr;
    U32 ulDramAddrSrc;
    PhysicalAddr FlashAddr = { 0 };
    RED *pRed;

    FlashAddr.m_PUSer = RootTableAddr[ucPageIndex].m_PUSer;
    FlashAddr.m_OffsetInSuperPage = RootTableAddr[ucPageIndex].m_OffsetInSuperPage;
    FlashAddr.m_BlockInPU = RootTableAddr[ucPageIndex].m_BlockInPU;
    FlashAddr.m_PageInBlock = RootTableAddr[ucPageIndex].m_PageInBlock;
    FlashAddr.m_LPNInPage = 0;

    ucSuperPuNum = FlashAddr.m_PUSer;
    ucLunInSuperPu = FlashAddr.m_OffsetInSuperPage;

    pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

    pRed = &(pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]);

    for (ucLoop = 0; ucLoop < RED_SW_SZ_DW; ucLoop++)
    {
        pRed->m_Content[ucLoop] = 0;
    }

    /* patch for upper level send saveRT command during table rebuild. */
    if (TRUE == L2_IsBootupOK())
    {
        L2_IncTimeStampInPu(ucSuperPuNum);
        pRed->m_RedComm.ulTimeStamp = L2_GetTimeStampInPu(ucSuperPuNum);
    }
    else
    {
        pRed->m_RedComm.ulTimeStamp = 0;
    }
    pRed->m_RedComm.DW1 = INVALID_8F;
    pRed->m_RedComm.bcBlockType = BLOCK_TYPE_RT;
    pRed->m_RedComm.bcPageType = PAGE_TYPE_RT;
    pRed->m_RedComm.eOPType = OP_TYPE_RT_WRITE;
    pRed->bPowerCycle = bPowerCycle;

    /* update PBIT 1st page Info */
    if (0 == FlashAddr.m_PageInBlock)
    {
        L2_Set_TableBlock_PBIT_Info(FlashAddr.m_PUSer, FlashAddr.m_OffsetInSuperPage, FlashAddr.m_BlockInPU, pRed);
    }

    if (TRUE != bRTSaveAgain)
    {
        L2_RTPrepareSysInfo();
    }

    //pRT may mallocat at SRAM, should copy pRT to tempbuffer
    ulDramAddrSrc = (U32)pRT + ucPageIndex*BUF_SIZE;
    ulBufferAddr = COM_GetMemAddrByBufferID((L2RTStartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
    COM_MemCpy((U32 *)ulBufferAddr, (U32 *)ulDramAddrSrc, BUF_SIZE / sizeof(U32));

    //FIRMWARE_LogInfo("MCU#%d L2_RT_Save_WritePage bPowerCycle 0x%x PU 0x%x LUN 0x%x BLK 0x%x PG 0x%x\n", HAL_GetMcuId(),
    //    bPowerCycle, FlashAddr.m_PUSer, FlashAddr.m_OffsetInSuperPage, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);

    L2_FtlWriteLocal(&FlashAddr, (U32 *)ulBufferAddr, (U32 *)pRed, &(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, NULL);

#ifdef L2MEASURE
    L2MeasureLogIncWCnt(ucSuperPuNum, L2MEASURE_TYPE_RT_SLC);
#endif

    return;
}

LOCAL MCU12_VAR_ATTR U8 ucRTSavePageIndex;

U32 MCU1_DRAM_TEXT L2_RT_SaveRT(BOOL bPowerCycle)
{
    U8  ucSuperPuNum;
    U8  ucLunInSuperPu;
    U32 ulRet;
    U32 ulCmdRet;
    U32 ucFlashStatus;
    PhysicalAddr InvalidFlashAddr;

    ulRet = L2_RT_WAIT;
    InvalidFlashAddr.m_PPN = INVALID_8F;

    switch (gRTSaveStatus)
    {
    case L2_RT_SAVE_START:
    {
        /* Init global values */
        if (TRUE == bPowerCycle)
        {
            g_pSubSystemDevParamPage->SafeShutdownCnt++;
        }

        /* Update RootTableAddr */
        L2_RT_Save_UpdateFlashAddr();

        bRTSaveAgain = FALSE;
        gRTSaveStatus = L2_RT_SAVE_WRITE_PAGE;
        ucRTSavePageIndex = 0;
    }
        break;

    case L2_RT_SAVE_WRITE_PAGE:
    {
        /* save RT page to Flash */
        ucSuperPuNum = RootTableAddr[ucRTSavePageIndex].m_PUSer;
        ucLunInSuperPu = RootTableAddr[ucRTSavePageIndex].m_OffsetInSuperPage;
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            L2_RT_Save_WritePage(bPowerCycle, ucRTSavePageIndex);
            gRTSaveStatus = L2_RT_SAVE_WAIT_PAGE;
        }
    }
        break;

    case L2_RT_SAVE_WAIT_PAGE:
    {
        /* wait for flash idle */
        ucSuperPuNum = RootTableAddr[ucRTSavePageIndex].m_PUSer;
        ucLunInSuperPu = RootTableAddr[ucRTSavePageIndex].m_OffsetInSuperPage;
        ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
        {
            U8 ucRootTablePageCount;

            /* check if need to erase previous block */
            ucRootTablePageCount = RT_PAGE_COUNT - 1;
            ucSuperPuNum = RootTableRecycleAddr.m_PUSer;
            ucLunInSuperPu = RootTableRecycleAddr.m_OffsetInSuperPage;
            if ((RootTableRecycleAddr.m_PPN != INVALID_8F)
                && (ucRTSavePageIndex >= ucRootTablePageCount))
            {
                if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                {
                    /* erase previous block */
                    pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

                    DBG_Printf("L2_RT_SaveRT ERASE PU 0x%x LUN 0x%x BLK 0x%x start\n", ucSuperPuNum, ucLunInSuperPu, RootTableRecycleAddr.m_BlockInPU);

                    L2_FtlEraseBlock(ucSuperPuNum, ucLunInSuperPu, RootTableRecycleAddr.m_BlockInPU,
                        (U8 *)&(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, FALSE);

                    gRTSaveStatus = L2_RT_SAVE_WAIT_ERASE;
                }
            }
            else
            {
                ucRTSavePageIndex++;
                if (ucRTSavePageIndex >= RT_PAGE_COUNT)
                {
                    gRTSaveStatus = L2_RT_SAVE_SUCCESS;
                }
                else
                {
                    gRTSaveStatus = L2_RT_SAVE_WRITE_PAGE;
                }
            }
        }
        else
        {
            /* save RT write page fail: errorhandling move block */
            ulCmdRet = L2_TableErrorHandlingMoveBlock(RootTableAddr[ucRTSavePageIndex].m_PUSer, RootTableAddr[ucRTSavePageIndex].m_OffsetInSuperPage,
                RootTableAddr[ucRTSavePageIndex].m_BlockInPU, RootTableAddr[ucRTSavePageIndex].m_PageInBlock, WRITE_ERR);

            if (L2_ERR_HANDLING_PENDING == ulCmdRet)
            {
                ;
            }
            else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
            {
                /* write again */
                U8 ucPgIndex;
                U16 usRTBlk = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, RT_BLOCK_ADDR_BASE, TRUE); //@@mavishuang: please do not use constant 2);

                for (ucPgIndex = 0; ucPgIndex < RT_PAGE_COUNT; ucPgIndex++)
                {
                    RootTableAddr[ucPgIndex].m_BlockInPU = usRTBlk;
                }
                bRTSaveAgain = TRUE;
                gRTSaveStatus = L2_RT_SAVE_WRITE_PAGE;
            }
            else
            {
                DBG_Printf("L2_RT_SaveRT PU 0x%x Erase ErrorHandling Fail\n", RootTableRecycleAddr.m_PUSer);
                DBG_Getch();
            }
        }
    }
        break;

    case L2_RT_SAVE_WAIT_ERASE:
    {
        /* wait for flash idle */
        ucSuperPuNum = RootTableRecycleAddr.m_PUSer;
        ucLunInSuperPu = RootTableRecycleAddr.m_OffsetInSuperPage;
        ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
        {

            DBG_Printf("L2_RT_SaveRT ERASE PU 0x%x LUN 0x%x BLK 0x%x success!\n", ucSuperPuNum, ucLunInSuperPu, RootTableRecycleAddr.m_BlockInPU);
            RootTableRecycleAddr.m_PPN = INVALID_8F;

            gRTSaveStatus = L2_RT_SAVE_SUCCESS;
        }
        else
        {
            /* save RT erase block fail: errorhandling move block */
            ulCmdRet = L2_TableErrorHandlingMoveBlock(RootTableRecycleAddr.m_PUSer, RootTableRecycleAddr.m_OffsetInSuperPage,
                RootTableRecycleAddr.m_BlockInPU, 0, ERASE_ERR);

            if (L2_ERR_HANDLING_PENDING == ulCmdRet)
            {
                ;
            }
            else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
            {
                /* write again */
                /* Update RootTableAddr */
                L2_RT_Save_UpdateFlashAddr();
                bRTSaveAgain = TRUE;
                gRTSaveStatus = L2_RT_SAVE_WRITE_PAGE;
                ucRTSavePageIndex = 0;
                RootTableRecycleAddr.m_PPN = INVALID_8F;
            }
            else
            {
                DBG_Printf("L2_RT_SaveRT PU 0x%x LUN 0x%x Erase ErrorHandling Fail\n", RootTableRecycleAddr.m_PUSer, RootTableRecycleAddr.m_OffsetInSuperPage);
                DBG_Getch();
            }
        }
    }
        break;

    case L2_RT_SAVE_FAIL:
    {
        ulRet = L2_RT_FAIL;
    }
        break;

    case L2_RT_SAVE_SUCCESS:
    {
        ulRet = L2_RT_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_RT_SaveRT invalid Status 0x%x\n", gRTSaveStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

/********************** FILE END ***************/

