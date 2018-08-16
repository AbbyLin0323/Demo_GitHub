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
File Name     : L2_FullRecovery.c
Version       : Initial version
Author        : henryluo
Created       : 2015/2/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2015/2/28
Author      : henryluo
Modification: Created file

******************************************************************************/
#include "HAL_Inc.h"
#include "HAL_ParamTable.h"
#include "L2_Defines.h"
#include "L2_Debug.h"
#include "L2_PMTManager.h"
#include "L2_Boot.h"
#include "L2_RT.h"
#include "L2_TableBlock.h"
#include "L2_Thread.h"
#include "L1_GlobalInfo.h"
#include "L2_Interface.h"
#include "COM_Memory.h"
#include "COM_BitMask.h"
#include "L2_TableBBT.h"
#include "L2_FCMDQ.h"
#include "FW_BufAddr.h"
#include "L2_Erase.h"

#ifdef SIM
#include "L2_SimTablecheck.h"
#endif

#ifndef SIM
#include <xtensa/tie/via.h>
#include <xtensa/tie/xt_core.h>
#include <xtensa/tie/xt_timer.h>
#else
extern void L2_CollectErrAddrOfCheckPMT(U8 ucSubSystemPu, U16 PhyBlk, U16 PhyPage);
#endif
#define TLC_UECCCNT_ERASE_THRESHOLD 48
GLOBAL  BOOL bTBRebuildPending;
GLOBAL  BOOL bTBRebuildEraseAll[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  U32  gulMaxTableBlockTS[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  PBIT *pPBIT_Temp[SUBSYSTEM_SUPERPU_MAX];


GLOBAL  BLOCK_TYPE DummyWriteBlkType;
GLOBAL  U16 g_ulTempPhyBlkStart[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
GLOBAL  U16 g_ulTempLunPhyBlk[LUN_NUM_PER_SUPERPU];
GLOBAL  BOOL bSaveBBT[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  BOOL bNeedSaveBBT[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
GLOBAL  U16 ulLunBlk[LUN_NUM_PER_SUPERPU];
GLOBAL  U32 ulLunBlkTS[LUN_NUM_PER_SUPERPU];
GLOBAL  BOOL ulNeedFreeBlkTargetLun[LUN_NUM_PER_SUPERPU];
GLOBAL  BOOL bIsUpdate[LUN_NUM_PER_SUPERPU]; 
LOCAL BOOL bNeedLoadPRMT[TARGET_ALL - 1];
LOCAL BOOL bHandleSPORDuringErase[SUBSYSTEM_SUPERPU_MAX];
LOCAL U8 SLCIndex[SUBSYSTEM_SUPERPU_MAX];
LOCAL U8 FirstPageEmptyAT1Index[SUBSYSTEM_SUPERPU_MAX];
LOCAL U16 AT1PartialWritePage[SUBSYSTEM_SUPERPU_MAX];
extern  GLOBAL  U32 g_L2TempBufferAddr;
extern  GLOBAL  U16 L2RTStartTempBufID;


extern U32 L2_TB_LoadGetFlashAddr(PAGE_TYPE PageType, U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPageIndex, PhysicalAddr* FlashAddr);
extern void L2_TB_UpdateCurBlockMaxTS(U8 ucSuperPuNum, U8 ucLunInSuperPu, RED *pRed);
extern BOOL HAL_GetPbnBindingTableEnFlag(void);
extern void  SwapTwoValue(U32* pValue1, U32* pValue2);

BOOL MCU1_DRAM_TEXT L2_IsSLCBlockType(BLOCK_TYPE blk_type)
{
    BOOL bSLCBlk;

    switch (blk_type)
    {
    case BLOCK_TYPE_SEQ:
        bSLCBlk = TRUE;
        break;
    case BLOCK_TYPE_RAN:
        bSLCBlk = TRUE;
        break;
    case BLOCK_TYPE_TLC_W:
        bSLCBlk = FALSE;
        break;
    case BLOCK_TYPE_TLC_GC:
        bSLCBlk = FALSE;
        break;

    default:
        DBG_Printf("Unknown block type %d\n", blk_type);
        DBG_Getch();
        break;
    }

    return bSLCBlk;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_RT_SetCurAT0Blk(U8 ucSuperPuNum, U16 BlocKNum, U8 ucLunInSuperPu)
{
    pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = BlocKNum;
    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_RT_SetCurAT0PPO(U8 ucSuperPuNum, U16 PPO)
{
    pRT->m_RT[ucSuperPuNum].CurAT0PPO = PPO;
    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_MarkBadBlock(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu;
    U16 usBlock;
    BOOL bBadBlk;
    U8 ucTLun;

    /* mark bad block */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu);
        for (usBlock = 0; usBlock < (BLK_PER_PLN + RSV_BLK_PER_PLN); usBlock++)
        {
            bBadBlk = L2_BbtIsGBbtBadBlock(ucTLun, usBlock);
            if (TRUE == bBadBlk)
            {
                L2_PBIT_Set_Error(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
            }
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_MarkAllTable(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu;
    U16 usBlock;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        /* mark GB block */
        L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, GLOBAL_BLOCK_ADDR_BASE, TRUE);

        /* mark BBT block - 1st none bad block from BBT base is BBT Block */
        usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum, BBT_BLOCK_ADDR_BASE, TRUE);
        L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);

        /* mark RT, AT0, AT1 and Trace block */
        L2_RT_Rebuild_MarkTable(ucSuperPuNum, ucLunInSuperPu);
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableLocateLatestPBIT(U8 ucSuperPuNum, RED *pRed, U8 ucLunInSuperPu)
{
    RED *pPBIT_Red;

    pPBIT_Red = L2_TB_GetRedAddr(PAGE_TYPE_PBIT, ucSuperPuNum, ucLunInSuperPu);

    if (pRed->m_PageIndex >= L2_TB_GetPageCntPerCE(PAGE_TYPE_PBIT))
    {
        DBG_Printf("SPU %d PBIT_PageIndex %d error!\n", ucSuperPuNum, pRed->m_PageIndex);
        DBG_Getch();
    }

    if (INVALID_4F == pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][pRed->m_PageIndex])
    {
        /* find 1st PBIT page, record infos */
        pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][pRed->m_PageIndex] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
        pRT->m_RT[ucSuperPuNum].PBITPPO[pRed->m_PageIndex] = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
        pPBIT_Red->m_RedComm.ulTimeStamp = pRed->m_RedComm.ulTimeStamp;
    }
    else
    {
        /* check TS for lastest PBIT page */
        if (pPBIT_Red->m_RedComm.ulTimeStamp < pRed->m_RedComm.ulTimeStamp)
        {
            pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][pRed->m_PageIndex] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            pRT->m_RT[ucSuperPuNum].PBITPPO[pRed->m_PageIndex] = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
            pPBIT_Red->m_RedComm.ulTimeStamp = pRed->m_RedComm.ulTimeStamp;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_RecordFlashEraseInfor(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu, ucFlashStatus;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        if (SUBSYSTEM_STATUS_SUCCESS != ucFlashStatus)
        {
            /* erase fail */
            pTBRebManagement->bIsEraseFail[ucSuperPuNum][ucLunInSuperPu] = TRUE;
        }
    }
}

void MCU1_DRAM_TEXT L2_TB_RecordFlashReadInfor(U8 ucSuperPuNum, L2_TB_REBUILD_STATUS *pStatus)
{
    U8  ucLunInSuperPu, ucFlashStatus;
    U16 usBlock;
    RED* pRed;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            if ((*pStatus == L2_TB_REBUILD_CHECK_FIRST_PAGE) || (*pStatus == L2_TB_REBUILD_RT_CHECK_FIRST_SUPER_PAGE))
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_Set_TableBlock_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
            }

            pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum]++;
            pTBRebManagement->usReadSuccessLun[ucSuperPuNum][ucLunInSuperPu] = TRUE;                
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum]++;
        }
        else
        {
            pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum]++;
        }
    }
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_WaitEraseFinish(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu, ucFlashStatus;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        /* bit = 1, means current Lun has send erase command to flash*/
        if (pTBRebManagement->ulLunEraseBitmap[ucSuperPuNum] & (1<<ucLunInSuperPu))
        {
            ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return FALSE; 
            }  
        }
    }

    /*calculate read Flash status count and set PBIT information*/
    L2_TB_Rebuild_RecordFlashEraseInfor(ucSuperPuNum);
    return TRUE;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_WaitAllFlashEraseFinish(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu, ucFlashStatus;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            return FALSE; 
        }
    }

    /*calculate read Flash status count and set PBIT information*/
	L2_TB_Rebuild_RecordFlashEraseInfor(ucSuperPuNum);
    return TRUE;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_WaitAllFlashFinish(U8 ucSuperPuNum, L2_TB_REBUILD_STATUS *pStatus)
{
    U8  ucLunInSuperPu, ucFlashStatus;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            return FALSE; 
        }
    }

    /*calculate read Flash status count and set PBIT information*/
    L2_TB_RecordFlashReadInfor(ucSuperPuNum, pStatus);
    return TRUE;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableEraseResetParam(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
         pTBRebManagement->bIsEraseFail[ucSuperPuNum][ucLunInSuperPu] = FALSE;
    }

    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
    pTBRebManagement->ulLunEraseBitmap[ucSuperPuNum] = 0;
}

void MCU1_DRAM_TEXT L2_RT_Rebuild_ResetParam(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pTBRebManagement->usReadSuccessLun[ucSuperPuNum][ucLunInSuperPu] = FALSE;
        pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
        pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
    }

    pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] = 0;
    pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] = 0;
    pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] = 0;
    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
    pTBRebManagement->ulTempLunBitmap[ucSuperPuNum] = 0;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableResetParam(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pTBRebManagement->usReadSuccessLun[ucSuperPuNum][ucLunInSuperPu] = FALSE;
    }

    pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] = 0;
    pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] = 0;
    pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] = 0;
    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
}

U16 MCU1_DRAM_TEXT L2_TB_Rebuild_GetPrevTargetAT1Blk(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U8 i;
    U16 usCurTargetAT1Blk, usBlk;
    U32 ulCurMaxTS = 0, ulTempTS;

	usBlk = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][0];
	ulTempTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usBlk].LastPageTimeStamp;
    usCurTargetAT1Blk = usBlk;

    for (i = 0; i < AT1_BLOCK_COUNT; i ++)
    {
        usBlk = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][i];
        ulTempTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usBlk].LastPageTimeStamp;
		DBG_Printf("[%s]Lun%d AT1blk %d TS 0x%x\n", __FUNCTION__, ucLunInSuperPu, usBlk, ulTempTS);
        if(ulTempTS > ulCurMaxTS)
        {
            usCurTargetAT1Blk = usBlk;
            ulCurMaxTS = ulTempTS;
        }
    }

	return usCurTargetAT1Blk;
}

U32 MCU1_DRAM_TEXT L2_TB_RecoverPbit_GetAt0PhyBlk(U8 ucSuperPuNum, U8 ucLun, U32 ulAt0Blk)
{
    return pPbitSearch->usAt0PhyBlk[ucSuperPuNum][ucLun][ulAt0Blk];
}

void MCU1_DRAM_TEXT L2_TB_RecoverPbit_Wait(U8 ucSuperPuNum)
{
    U8 ucLun;
    U8 ucFlashStatus;
    U32 ulBitMapMsk = 0;

    for (ucLun = 0; ucLun < LUN_NUM_PER_SUPERPU; ucLun++)
    {
        ucFlashStatus = pPbitSearch->ucFlashStatus[ucSuperPuNum][ucLun];
        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            break;
        }
        else if ((SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus) || (SUBSYSTEM_STATUS_RECC == ucFlashStatus) || (SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus))
        {
            ulBitMapMsk |= (1 << ucLun);
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_CMD_EMPTYPG;
            break;
        }
        else if (SUBSYSTEM_STATUS_FAIL == ucFlashStatus)
        {
            pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_CMD_FAIL;
            break;
        }
        else
        {
            DBG_Printf("L2_TB_RecoverPbit_Wait Invalid flash status:0x%x\n", ucFlashStatus);
            DBG_Getch();
        }
    }
    if (SUPERPU_LUN_NUM_BITMSK == ulBitMapMsk)
    {
        pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_CMD_SUCCESS;
    }
}
void MCU1_DRAM_TEXT L2_TB_RecoverPbit_CmdSend(U8 ucSuperPuNum)
{
    U8 ucLun;
    BOOL blLunCmdAlreadySend;
    PhysicalAddr FlashAddr = { 0 };

    for (ucLun = 0; ucLun < LUN_NUM_PER_SUPERPU; ucLun++)
    {
        blLunCmdAlreadySend = (pPbitSearch->ucBitMapMsk[ucSuperPuNum] & (1 << ucLun));
        if ((TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLun))) && (!blLunCmdAlreadySend))
        {
            FlashAddr.m_PUSer = ucSuperPuNum;
            FlashAddr.m_BlockInPU = L2_TB_RecoverPbit_GetAt0PhyBlk(ucSuperPuNum, ucLun, pPbitSearch->usAt0LogicBlk[ucSuperPuNum]);
            FlashAddr.m_PageInBlock = pPbitSearch->usAt0LogicPage[ucSuperPuNum];
            FlashAddr.m_OffsetInSuperPage = ucLun;
            L2_FtlReadLocal((U32*)L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLun), &FlashAddr, &(pPbitSearch->ucFlashStatus[ucSuperPuNum][ucLun]), (U32*)&pPbitSearch->PbitPuRed[ucSuperPuNum][ucLun], LPN_PER_BUF, 0, TRUE, TRUE);
            pPbitSearch->ucBitMapMsk[ucSuperPuNum] |= (1 << ucLun);
        }
    }
    if (pPbitSearch->ucBitMapMsk[ucSuperPuNum] == SUPERPU_LUN_NUM_BITMSK)
    {
        pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_CMD_WAIT;
        pPbitSearch->ucBitMapMsk[ucSuperPuNum] = 0;
    }

}

void L2_Tb_RecoverPbit_Update_Pos(U8 ucSuperPuNum)
{
    if (pPbitSearch->usAt0LogicPage[ucSuperPuNum] == (PG_PER_SLC_BLK - 1))
    {
        pPbitSearch->usAt0LogicBlk[ucSuperPuNum]++;
        pPbitSearch->usAt0LogicPage[ucSuperPuNum] = 0;
        pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_CMD_SEND;
    }
    else
    {
        pPbitSearch->usAt0LogicPage[ucSuperPuNum]++;
        pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_CMD_SEND;
    }
    if (pPbitSearch->usAt0LogicBlk[ucSuperPuNum] == (AT0_BLOCK_COUNT))
    {
        pPbitSearch->usAt0LogicBlk[ucSuperPuNum] = 0;
        pPbitSearch->usAt0LogicPage[ucSuperPuNum] = 0;

        pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_SEARCH_DONE;
    }
}
BOOL L2_TB_RecoverPbit_Is_Pbit(U8 ucSuperPuNum)
{
    U8 ucLun;
    BOOL blRes;
    blRes = TRUE;
    for (ucLun = 0; ucLun < LUN_NUM_PER_SUPERPU; ucLun++)
    {
        if (pPbitSearch->PbitPuRed[ucSuperPuNum][ucLun].m_RedComm.bcPageType != PAGE_TYPE_PBIT)
        {
            blRes = FALSE;
            break;
        }
    }
    return blRes;
}

void L2_TB_RecoverPbit_Copy_Data(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu = 0;
    U32 ulLength;
    U32 ulTargetAddr, ulBufferAddr;
    U32 usAt0LogicBlk = pPbitSearch->usAt0LogicBlk[ucSuperPuNum];

    //DBG_Printf("MCU%d RecoverPBIT Pu:%d At0Blk:%d Pg:%d TS:%d PbitPageIndex %d \n", HAL_GetMcuId(), ucSuperPuNum, pPbitSearch->usAt0PhyBlk[ucSuperPuNum][0][usAt0LogicBlk],
    //    pPbitSearch->usAt0LogicPage[ucSuperPuNum], pPbitSearch->ulAt0PbitMaxTsNum[ucSuperPuNum], pPbitSearch->usPbitPageIndex[ucSuperPuNum]);

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
        ulTargetAddr = L2_TB_GetDataAddr(PAGE_TYPE_PBIT, ucSuperPuNum, ucLunInSuperPu, pPbitSearch->usPbitPageIndex[ucSuperPuNum]);
        ulLength = L2_TB_GetDataLength(PAGE_TYPE_PBIT, ucSuperPuNum, ucLunInSuperPu, pPbitSearch->usPbitPageIndex[ucSuperPuNum]);
        COM_MemCpy((U32*)ulTargetAddr, (U32*)ulBufferAddr, ulLength / 4);

        //FIRMWARE_LogInfo("L2_TB_RecoverPbit_Copy_Data Pu:%d At0Blk:%d Pg:%d TS:%d TargetAddr 0x%x DramAddr 0x%x Len 0x%x PBITIndex %x\n", 
        //    ucSuperPuNum, pPbitSearch->usAt0PhyBlk[ucSuperPuNum][0][usAt0LogicBlk],
        //    pPbitSearch->usAt0LogicPage[ucSuperPuNum], pPbitSearch->ulAt0PbitMaxTsNum[ucSuperPuNum], ulTargetAddr, ulBufferAddr, ulLength, pPbitSearch->usPbitPageIndex[ucSuperPuNum]);
    }

    pPbitSearch->usPbitPageIndex[ucSuperPuNum]++;
    if (pPbitSearch->usPbitPageIndex[ucSuperPuNum] >= L2_TB_GetPageCntPerCE(PAGE_TYPE_PBIT))
    {
        pPbitSearch->usPbitPageIndex[ucSuperPuNum] = 0;
        pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_SEARCH_DONE;
    }
    else
    {
        pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = L2_TB_PBIT_CMD_SEND;
    }
}

BOOL L2_TB_RecoverPbit_Is_Max_Ts(U8 ucSuperPuNum)
{
    BOOL blRet = FALSE;

    if (pPbitSearch->PbitPuRed[ucSuperPuNum][0].m_RedComm.ulTimeStamp > pPbitSearch->ulAt0PbitMaxTsNum[ucSuperPuNum])
    {
        pPbitSearch->ulAt0PbitMaxTsNum[ucSuperPuNum] = pPbitSearch->PbitPuRed[ucSuperPuNum][0].m_RedComm.ulTimeStamp;
        blRet = TRUE;
    }
    return blRet;
}

BOOL MCU1_DRAM_TEXT L2_TB_RecoverPbit_Process()
{
    U8 ucSuperPuNum;
    BOOL blRet = TRUE;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        switch (pPbitSearch->ucPbitPuStatus[ucSuperPuNum])
        {
        case L2_TB_PBIT_CMD_SEND:
            L2_TB_RecoverPbit_CmdSend(ucSuperPuNum);
            break;

        case L2_TB_PBIT_CMD_WAIT:
            L2_TB_RecoverPbit_Wait(ucSuperPuNum);
            break;

        case L2_TB_PBIT_CMD_SUCCESS:
            if (L2_TB_RecoverPbit_Is_Pbit(ucSuperPuNum))
            {
                if (L2_TB_RecoverPbit_Is_Max_Ts(ucSuperPuNum))
                {
                    L2_TB_RecoverPbit_Copy_Data(ucSuperPuNum);
                }
            }
            L2_Tb_RecoverPbit_Update_Pos(ucSuperPuNum);
            break;

        case L2_TB_PBIT_CMD_EMPTYPG:
            pPbitSearch->usAt0LogicBlk[ucSuperPuNum]++;
            pPbitSearch->usAt0LogicPage[ucSuperPuNum] = 0;
            pPbitSearch->ucPbitPuStatus[ucSuperPuNum] = (pPbitSearch->usAt0LogicBlk[ucSuperPuNum] == AT0_BLOCK_COUNT ? L2_TB_PBIT_SEARCH_DONE : L2_TB_PBIT_CMD_SEND);
            break;

        case L2_TB_PBIT_CMD_FAIL:
            L2_Tb_RecoverPbit_Update_Pos(ucSuperPuNum);
            break;

        case L2_TB_PBIT_SEARCH_DONE:
            break;
        }
    }

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        if (pPbitSearch->ucPbitPuStatus[ucSuperPuNum] != L2_TB_PBIT_SEARCH_DONE)
        {
            blRet = FALSE;
        }
    }

    return blRet;
}

void MCU1_DRAM_TEXT L2_TB_RecoverPbit_Init()
{
    U8 ucSuperPu;
    pPbitSearch->FindPbitStatus = L2_TB_RECOVER_PBIT_INIT;


    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        pPbitSearch->usAt0LogicBlk[ucSuperPu] = 0;
        pPbitSearch->usAt0LogicPage[ucSuperPu] = 0;
        pPbitSearch->ulAt0PbitMaxTsNum[ucSuperPu] = 0;
        pPbitSearch->ucBitMapMsk[ucSuperPu] = 0;
        pPbitSearch->ucPbitPuStatus[ucSuperPu] = L2_TB_PBIT_CMD_SEND;
        pPbitSearch->usPbitPageIndex[ucSuperPu] = 0;

    }

    L2_PBIT_Init_Clear_All();

    if (TRUE == HAL_GetInheritEraseCntEnFlag())
    {
        pPbitSearch->FindPbitStatus = L2_TB_RECOVER_PBIT_LOAD_PBN_BINDING_TABLE;
    }
    else
    {
        pPbitSearch->FindPbitStatus = L2_TB_RECOVER_PBIT_DONE;
    }
}

BOOL MCU1_DRAM_TEXT L2_TB_RecoverPbit_Locate_At0(BOOL * blIsBbtExist)
{
    BOOL blRet = FALSE;
    U8 ucSuperPu, ucAt0Blk, ucLun;

    U32 ulBlk, ulStartBlk;

    if (TRUE != HAL_GetInheritEraseCntEnFlag())
    {
        return TRUE;
    }

    blRet = L2_BbtLoad(blIsBbtExist);
    if (blRet == TRUE)
    {
        if ((*blIsBbtExist) == TRUE)
        {
            ulStartBlk = (GLOBAL_BLOCK_COUNT + BBT_BLOCK_COUNT + RT_BLOCK_COUNT);
            for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
            {
                for (ucLun = 0; ucLun < LUN_NUM_PER_SUPERPU; ucLun++)
                {
                    ucAt0Blk = 0;
                    for (ulBlk = ulStartBlk; ulBlk < (ulStartBlk + AT0_BLOCK_COUNT); ulBlk++)
                    {
                        pPbitSearch->usAt0PhyBlk[ucSuperPu][ucLun][ucAt0Blk] = (L2_RT_Search_BlockBBT(ucSuperPu*LUN_NUM_PER_SUPERPU + ucLun, ulBlk, TRUE));
                        //DBG_Printf("MCU#%d SuperPu:%d LUN:%d AT0Blk %d PhyBlk:%d\n", HAL_GetMcuId(),ucSuperPu, ucLun, ucAt0Blk, pPbitSearch->usAt0PhyBlk[ucSuperPu][ucLun][ucAt0Blk]);
                        ucAt0Blk++;
                    }
                }
            }
        }
    }
    return blRet;
}

BOOL MCU1_DRAM_TEXT L2_TB_RecoverPbit()
{
    BOOL blRet = FALSE;
    BOOL blIsBbtExist = FALSE;
    BOOL bExistHardBindTable = TRUE;

    switch (pPbitSearch->FindPbitStatus)
    {
    case L2_TB_RECOVER_PBIT_LOAD_PBN_BINDING_TABLE:
        // please note that if we want to inherit PBIT information,
        // we must load the PBN binding table first and inherit it too,
        // otherwise:
        // a. we may load erroneous data due to faulty PBN mapping
        // b. the content of PBIT may be incorrect since the original mapping is lost
        if (L2_BbtLoadPbnBindingTable() == TRUE)
        {
            // proceed only if the PBN binding table exist
            if(L2_BbtPbnBindingTableExistenceCheck(&bExistHardBindTable) == TRUE)
            {
                BOOL bPTableSoftBindingFlag = HAL_GetPbnBindingTableEnFlag();
                BOOL bExistSoftBindTable =  !bExistHardBindTable;

                if((bPTableSoftBindingFlag == TRUE) && (bExistSoftBindTable == FALSE))
                {
                    pPbitSearch->FindPbitStatus = L2_TB_RECOVER_PBIT_DONE;
                    DBG_Printf("L2_TB_RecoverPbit: PBN binding table already exist, Hard binding: %d  Need transfer table, do not recover PBIT! \n", bExistHardBindTable);
                }
                else
                {
                    // enable the PBN binding table
                    L2_BbtEnablePbnBindingTable();
                    DBG_Printf("L2_TB_RecoverPbit: PBN binding table already exist, Hard binding: %d \n", bExistHardBindTable);
                    pPbitSearch->FindPbitStatus = L2_TB_RECOVER_PBIT_FIND_AT0;
                }
                L2_BbtMemZeroGBbt();
            }
            else
            {
                pPbitSearch->FindPbitStatus = L2_TB_RECOVER_PBIT_DONE;
            }
        }
        break;
    case L2_TB_RECOVER_PBIT_FIND_AT0:
        if (L2_TB_RecoverPbit_Locate_At0(&blIsBbtExist) == FALSE)
        {
            ;
        }
        else
        {
            pPbitSearch->FindPbitStatus = (blIsBbtExist == TRUE ? L2_TB_RECOVER_PBIT_PROCESS : L2_TB_RECOVER_PBIT_DONE);
        }
        break;
    case L2_TB_RECOVER_PBIT_PROCESS:
        if (L2_TB_RecoverPbit_Process() == TRUE)
        {
            pPbitSearch->FindPbitStatus = L2_TB_RECOVER_PBIT_DONE;
        }
        break;
    case L2_TB_RECOVER_PBIT_DONE:
        // disable the PBN binding table 
        L2_BbtDisablePbnBindingTable();
        blRet = TRUE;
        break;
    }
    return blRet;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_TableRollBackEraseCnt(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu = 0;
    U32 ulLength;
    U32 ulTargetAddr, ulBufferAddr;
    BOOL bRet = FALSE;
    U16  usBlock;
    U8 i;
    U8 ucTLun; 
    
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
        ulTargetAddr = (U32)pPBIT_Temp[ucSuperPuNum] + (pTBRebManagement->usPageIndex[ucSuperPuNum][0] * LUN_NUM_PER_SUPERPU + ucLunInSuperPu)*BUF_SIZE;

        ulLength = L2_TB_GetDataLength(PAGE_TYPE_PBIT, ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
        COM_MemCpy((U32*)ulTargetAddr, (U32*)ulBufferAddr, ulLength / 4);

        //FIRMWARE_LogInfo("Load LatestPBIT Pu:%d TargetAddr 0x%x DramAddr 0x%x Len 0x%x PBITIndex %x\n", ucPuNum, ulTargetAddr, ulBufferAddr, ulLength, pTBManagement->usPageIndex[ucPuNum]);
    }

    pTBRebManagement->usPageIndex[ucSuperPuNum][0]++;
    if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] >= L2_TB_GetPageCntPerCE(PAGE_TYPE_PBIT))
    {
        pTBRebManagement->usPageIndex[ucSuperPuNum][0] = 0;

        for (i = 0; i < BLKTYPE_ALL; i++)
        {
            pPBIT[ucSuperPuNum]->m_EraseCnt[i] = pPBIT_Temp[ucSuperPuNum]->m_EraseCnt[i];
            pPBIT[ucSuperPuNum]->m_SaveEraseCnt[i] = pPBIT_Temp[ucSuperPuNum]->m_SaveEraseCnt[i];
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            for (usBlock = 0; usBlock < (BLK_PER_PLN + RSV_BLK_PER_PLN); usBlock++)
            {
                pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].EraseCnt = pPBIT_Temp[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].EraseCnt;
            }
        }
        
       if (ucSuperPuNum == 0)
       {
          for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
          {
               g_ptFMUserMgr[ucTLun].ulErsTime = pPBIT[0]->m_EraseCnt[BLKTYPE_TLC] / LUN_NUM_PER_SUPERPU;
          }
       }

        bRet = TRUE;
    }

    return bRet;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableCheckRT(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U8 AT0Offset;

    if (INVALID_4F == pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu])
    {
        for (AT0Offset = 0; AT0Offset < AT0_BLOCK_COUNT; AT0Offset++)
        {
            if (TRUE != pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][AT0Offset])
            {
                pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][AT0Offset];
            }
        }
        pRT->m_RT[ucSuperPuNum].CurAT0PPO = 0;
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_RecordDummyWriteInfor(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu, ucFlashStatus;
	
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
        {
            continue;
        }

        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum]++;
            pTBRebManagement->usReadSuccessLun[ucSuperPuNum][ucLunInSuperPu] = TRUE;                
        }
    }
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_FillDummyWriteInfor(U8 ucSuperPuNum, RED* DummyWriteSpare)
{
    DummyWriteSpare->m_RedComm.bcBlockType = DummyWriteBlkType;
    DummyWriteSpare->m_RedComm.bcPageType = PAGE_TYPE_DUMMY;
    DummyWriteSpare->m_RedComm.eOPType = OP_TYPE_DUMMY_WRITE;
    DBG_Printf("dummy wr prepare:blktype %d\n", DummyWriteBlkType);
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_WaitDummyWrite(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu;
    U8  ucFlashStatus;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum] & (1<<ucLunInSuperPu))
        {
            continue;
        }

        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            return FALSE;
        }
    }

    /*calculate dummy write Flash status count*/
    L2_TB_Rebuild_RecordDummyWriteInfor(ucSuperPuNum);
    return TRUE;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableUpdateMaxTS(U8 ucSuperPuNum, RED *pRed)
{
    if (pRed->m_RedComm.ulTimeStamp > gulMaxTableBlockTS[ucSuperPuNum])
    {
        gulMaxTableBlockTS[ucSuperPuNum] = pRed->m_RedComm.ulTimeStamp;
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableUpdateBlockTS(U8 ucSuperPuNum)
{
    if (gulMaxTableBlockTS[ucSuperPuNum] > pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
    {
        pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp = gulMaxTableBlockTS[ucSuperPuNum];
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableCheckRsvdBlock(RED* pRed, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16  usBlock;
    PAGE_TYPE PageType;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    PageType = pRed->m_RedComm.bcPageType;

    if (PAGE_TYPE_DATA == PageType)
    {
        g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu] = usBlock;
    }
    else
    {
        g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu] = usBlock;
    }

    return;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableOthersCaseNextStage(U8 ucSuperPuNum)
{
    U16 usBlock;
    L2_TB_REBUILD_STATUS NextStatus;

    /* use Lun 0 block index to check which block type current block is.
           AT1: set next empty page is PPO and current block finish
           trace block: current block finish */
    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][0];

    if ((usBlock >= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][AT1_BLOCK_COUNT - 1]))
    {
        pRT->m_RT[ucSuperPuNum].CurAT1PPO = pTBRebManagement->usPageIndex[ucSuperPuNum][0] + 1;
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
    else if ((usBlock >= pRT->m_RT[ucSuperPuNum].TraceBlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].TraceBlockAddr[0][TRACE_BLOCK_COUNT - 1]))
    {
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
    else
    {
        DBG_Printf("[%s]SPU%d CurBlk%d no match block type range\n", __FUNCTION__, ucSuperPuNum, usBlock);
        DBG_Getch();
    }

    return NextStatus;
}

PAGE_TYPE MCU1_DRAM_TEXT L2_TB_Rebuild_GetBlockType(U8 ucSuperPuNum)
{
    U16 usBlock;
    PAGE_TYPE PageType;

    /* No Red information, so use Lun 0 block index to check which page type current block is */
    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][0];

    if ((usBlock >= pRT->m_RT[ucSuperPuNum].AT0BlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].AT0BlockAddr[0][AT0_BLOCK_COUNT - 1]))
    {
        /* use PBIT to indicate AT0 block */
        PageType = PAGE_TYPE_PBIT;
    }
    else if ((usBlock >= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][AT1_BLOCK_COUNT - 1]))
    {
        PageType = PAGE_TYPE_PMT;
    }
    else if ((usBlock >= pRT->m_RT[ucSuperPuNum].TraceBlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].TraceBlockAddr[0][TRACE_BLOCK_COUNT - 1]))
    {
        PageType = PAGE_TYPE_TRACE;
    }
    else
    {
        DBG_Printf("[%s]SPU%d CurBlk%d no match block type range\n", __FUNCTION__, ucSuperPuNum, usBlock);
        DBG_Getch();
    }

    return PageType;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_NeedEraseBlock(U8 ucSuperPuNum)
{
    U16 usBlock;
    BOOL bNeedErase;

    /* use Lun 0 block index to search which block type current block is in RT.
           AT0:erase, AT1/trace blk: not erase */
    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][0];

    if ((usBlock >= pRT->m_RT[ucSuperPuNum].AT0BlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].AT0BlockAddr[0][AT0_BLOCK_COUNT - 1]))
    {
        bNeedErase = TRUE;
    }
    else if ((usBlock >= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][AT1_BLOCK_COUNT - 1]))
    {
        bNeedErase = TRUE;
    }
    else if ((usBlock >= pRT->m_RT[ucSuperPuNum].TraceBlockAddr[0][0])
        && (usBlock <= pRT->m_RT[ucSuperPuNum].TraceBlockAddr[0][TRACE_BLOCK_COUNT - 1]))
    {
        bNeedErase = FALSE;
    }
    else
    {
        DBG_Printf("[%s]SPU%d CurBlk%d no match block type range\n", __FUNCTION__, ucSuperPuNum, usBlock);
        DBG_Getch();
    }

    return bNeedErase;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TablePrepareEraseBitmap(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucFlashStatus;

    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = SUPERPU_LUN_NUM_BITMSK;
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
		pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
        if (SUBSYSTEM_STATUS_EMPTY_PG != ucFlashStatus)
        {
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucLunInSuperPu);
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
        }
    }

    return L2_TB_REBUILD_ERROR_ERASE_ONE_BLOCK;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TablePrepareDummyWriteInfor(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucFlashStatus;
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    /* first page all success, so we can only use Lun 0 index to get the page type inforamtion to decide dummy write block Red information */
    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif        
    case PAGE_TYPE_DPBM:
    case PAGE_TYPE_PMTI:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    {
        /* corresponding bit of needing dummy write Lun is 0 */
        pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum] = 0;
        pTBRebManagement->ulTempLunBitmap[ucSuperPuNum] = 0;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_EMPTY_PG != ucFlashStatus)
            {
                pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
            }
            else
            {
                DBG_Printf("Lun %d blk %d pg %d need dummy wr\n", ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 
	                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);
            }
        }
        NextStatus = L2_TB_REBUILD_DUMMY_WRITE;
        DummyWriteBlkType = BLOCK_TYPE_AT0;
    }
        break;
    case PAGE_TYPE_PMT:
    {
        /* AT1 don't need to dummy write, if current page is last page, do nothing.
            Otherwise, set next page is AT1 target block PPO */
        AT1PartialWritePage[ucSuperPuNum] = pTBRebManagement->usPageIndex[ucSuperPuNum][0];
        if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < PG_PER_SLC_BLK - 1)
        {
            DBG_Printf("AT1 last page %d is partial write, not need dummy write\n", AT1PartialWritePage[ucSuperPuNum]);
			NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            DBG_Printf("AT1 last super page is partial write, not need dummy write\n");
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
        
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU dummy write fill blocktype error, PageType%d\n", ucSuperPuNum, PageType);
        DBG_Getch();
	}
        break;
    }

    return NextStatus;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableBlkSetTS(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucFlashStatus;
    U16 usCurPBN;
    U32 ulCurBlkTS;
    PAGE_TYPE PageType;
    RED* pRed;

    /* check whether current page has dummy data, if has, current table page TS cannot be trust */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        usCurPBN = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
        if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            PageType = pRed->m_RedComm.bcPageType;
            if (PageType == PAGE_TYPE_DUMMY)
            {
                return;
            }
        }
    }

    /* update table block latest TS */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
        ulCurBlkTS = pRed->m_RedComm.ulTimeStamp;
        usCurPBN = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

        if(ulCurBlkTS > pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usCurPBN].LastPageTimeStamp)
        {
            pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usCurPBN].LastPageTimeStamp = ulCurBlkTS;
        }
    }
}

PAGE_TYPE MCU1_DRAM_TEXT L2_TB_Rebuild_TableGetPageTypeFromRED(U8 ucSuperPuNum, BLOCK_TYPE *ucCurBlkType)
{
    U8 ucLunInSuperPu, ucFlashStatus;
    BOOL bHasDummy = FALSE;
    PAGE_TYPE PageType = PAGE_TYPE_RSVD;
    RED* pRed;

    /* if current super page has any dummy write date, we need to use block type to check what is current scanning table block */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            PageType = pRed->m_RedComm.bcPageType;
            if (PageType == PAGE_TYPE_DUMMY)
            {
                bHasDummy = TRUE;
                *ucCurBlkType = pRed->m_RedComm.bcBlockType;
                DBG_Printf("[%s]Lun %d blk %d page %d has dummy pg\n", __FUNCTION__, ucLunInSuperPu,
                    pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);
                break;
            }
        }
    }

    if (bHasDummy == FALSE)
    {
        if ((PageType != PAGE_TYPE_PBIT) && (PageType != PAGE_TYPE_VBT) && (PageType != PAGE_TYPE_RPMT) && (PageType != PAGE_TYPE_PMTI)
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    #ifdef LCT_VALID_REMOVED
            && (PageType != PAGE_TYPE_PMT) && (PageType != PAGE_TYPE_DPBM) && (PageType != PAGE_TYPE_TLCW))
    #else
			&& (PageType != PAGE_TYPE_VBMT) && (PageType != PAGE_TYPE_PMT) && (PageType != PAGE_TYPE_DPBM) && (PageType != PAGE_TYPE_TLCW))
    #endif
#else//SHUTDOWN_IMPROVEMENT_STAGE1
    #ifdef LCT_VALID_REMOVED
			&& (PageType != PAGE_TYPE_PMT) && (PageType != PAGE_TYPE_DPBM))
    #else
			&& (PageType != PAGE_TYPE_VBMT) && (PageType != PAGE_TYPE_PMT) && (PageType != PAGE_TYPE_DPBM))
    #endif
#endif
        {
            DBG_Printf("[%s] some Lun pg %d read success, but no vaild pagetype %d error, read success Cnt %d, read UECC Cnt %d read Empty Cnt %d\n", 
				__FUNCTION__, pTBRebManagement->usPageIndex[ucSuperPuNum][0], PageType, pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum],
                pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum], pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum]);
            DBG_Getch();
        }
        return PageType;
    }
    else
    {
        return PAGE_TYPE_DUMMY;
    }
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableCheckFirstPage(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    U16 usBlock;
    RED* pRed;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlkType = BLOCK_TYPE_EMPTY;
    L2_TB_REBUILD_STATUS NextStatus;

    PageType = L2_TB_Rebuild_TableGetPageTypeFromRED(ucSuperPuNum, &BlkType);

    /* read first page all success, need update table block TS */
    L2_TB_Rebuild_TableBlkSetTS(ucSuperPuNum);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif
    case PAGE_TYPE_DPBM:
    case PAGE_TYPE_PMTI:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    {
        /* check AT0 block validation */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

            if ((usBlock < pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][0])
                || (usBlock > pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][AT0_BLOCK_COUNT - 1]))
            {
                DBG_Printf("[%s]SPU%d AT0Blk%d Wrong range%d-%d\n", __FUNCTION__, ucSuperPuNum, usBlock,
                    pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][0],
                    pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][AT0_BLOCK_COUNT - 1]);
                //DBG_Getch();

                if (bTBRebuildEraseAll[ucSuperPuNum] == FALSE)
                {
                    bTBRebuildEraseAll[ucSuperPuNum] = TRUE;
                }

            }

            /* locate latest PBIT for roll back erase count */
            if (PAGE_TYPE_PBIT == PageType)
            {
                L2_TB_Rebuild_TableLocateLatestPBIT(ucSuperPuNum, pRed, ucLunInSuperPu);
            }

            L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, pRed);
        }

        NextStatus = L2_TB_REBUILD_READ_LAST_PAGE;
    }
        break;
    case PAGE_TYPE_PMT:
    case PAGE_TYPE_DUMMY:
    {		
        if ((PageType == PAGE_TYPE_DUMMY) && (BlkType != BLOCK_TYPE_PMT))
        {
            DBG_Printf("[%s]SPU%d 1st page has dummy,but blk type 0x%x error\n", __FUNCTION__, ucSuperPuNum, BlkType);
            DBG_Getch();
        }

        /* check AT1 block validation */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

            /* check AT1 block validation */
            if ((usBlock < pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][0])
                || (usBlock > pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][AT1_BLOCK_COUNT - 1]))
            {
                DBG_Printf("[%s] SuperPU 0x%x AT1 Block 0x%x Wrong 0x%x 0x%x\n", __FUNCTION__,
                    ucSuperPuNum, usBlock, pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][0],
                    pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][AT1_BLOCK_COUNT - 1]);

                if (bTBRebuildEraseAll[ucSuperPuNum] == FALSE)
                {
                    bTBRebuildEraseAll[ucSuperPuNum] = TRUE;
                }
            }
        }

        NextStatus = L2_TB_REBUILD_READ_LAST_PAGE;
    }
        break;
    case PAGE_TYPE_TRACE:
    {
        /* check validation and just finish for Trace block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

            /* check validation and just finish for Trace block */
            if ((usBlock < pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][0])
                || (usBlock > pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][TRACE_BLOCK_COUNT - 1]))
            {
                DBG_Printf("[%s] PU 0x%x Trace Block 0x%x Wrong 0x%x 0x%x\n", __FUNCTION__, ucSuperPuNum,
                    usBlock, pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][0],
                    pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][TRACE_BLOCK_COUNT - 1]);

                if (bTBRebuildEraseAll[ucSuperPuNum] == FALSE)
                {
                    bTBRebuildEraseAll[ucSuperPuNum] = TRUE;
                }
            }
        }

        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid BlkType0x%x PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, BlkType, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableFirstPageAllUECC(U8 ucSuperPuNum)
{
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    /* use CurBlk index to search page type in RT, we use PBIT to represent the AT0 block */
    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);

    if ((PageType == PAGE_TYPE_PBIT) || (PageType == PAGE_TYPE_PMT))
    {
        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] ++;
        NextStatus = L2_TB_REBUILD_READ_LAST_PAGE;
    }
    else
    {
        /* trace block: current block finish */
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableFirstPageSuccessandUECC(U8 ucSuperPuNum)
{
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    /* new method: use Curblk index to search block type from RT */
    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);

    if ((PageType == PAGE_TYPE_PBIT) || (PageType == PAGE_TYPE_PMT))
    {
        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] ++;
        NextStatus = L2_TB_REBUILD_READ_LAST_PAGE;
    }
    else
    {
        /* trace block: current block finish */
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableCheckLastPage(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    U16 usBlock;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlkType = BLOCK_TYPE_EMPTY;
    L2_TB_REBUILD_STATUS NextStatus;
    U8 AT0Offset, i;
    RED* pRed;

    /* last page all read success has two case :
        case 1: all success data is valid - > orginal path
        case 2: any read success Lun page is dummy page -> according to the block type of dummy page red, 
        select the next stage */
    PageType = L2_TB_Rebuild_TableGetPageTypeFromRED(ucSuperPuNum, &BlkType);

    /* read last page all success, need update table block TS */
    L2_TB_Rebuild_TableBlkSetTS(ucSuperPuNum);
	
    AT0Offset = INVALID_2F;

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif        
    case PAGE_TYPE_DPBM:
    case PAGE_TYPE_PMTI:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            PageType = pRed->m_RedComm.bcPageType;

            /* locate latest PBIT for roll back erase count */
            if (PAGE_TYPE_PBIT == PageType)
            {
                L2_TB_Rebuild_TableLocateLatestPBIT(ucSuperPuNum, pRed, ucLunInSuperPu);
            }

            /* mark AT0 block for Erase */
            for (i = 0; i < AT0_BLOCK_COUNT; i++)
            {
                if (usBlock == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][i])
                {
                    AT0Offset = i;
                    break;
                }
            }

            if (AT0Offset == INVALID_2F)
            {
                DBG_Printf("[%s]PU:%d,AT0Offset error :0x%x\n", __FUNCTION__, ucSuperPuNum, AT0Offset);
                DBG_Getch();
            }
            pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][AT0Offset] = TRUE;
            /* reset Page index and start to read next Page for roll back erase count */
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
        }

        NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, pRed);
    }
        break;
    case PAGE_TYPE_PMT:
    {
        /* just finish, cannot be current block */
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
        break;
    case PAGE_TYPE_DUMMY:
    {
        /* read last page case occurs dummy data, it means that it may be Success + empty before */
        if (BlkType == BLOCK_TYPE_EMPTY)
        {
            DBG_Printf("[%s]SPU%d dummypage%d has empty blktype%d error\n", __FUNCTION__, ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }

        if (BlkType == BLOCK_TYPE_AT0)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                /* reset Page index and start to check next Page */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
            }

            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else if (BlkType == BLOCK_TYPE_PMT)
        {
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
        else
        {
            DBG_Printf("[%s]SPU %d last page dummypage%d has error blktype%d\n", __FUNCTION__, ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableLastPageAllUECC(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    /* use Lun 0 index to get the block and page type inforamtion in PBIT, which is recorded on reading first super page all success */
    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        /* reset Page index and start to check next Page */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
        }

        NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
    }
        break;
    case PAGE_TYPE_PMT:
    {
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableLastPageNoEmpty(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlkType = BLOCK_TYPE_EMPTY;
    L2_TB_REBUILD_STATUS NextStatus;

    /* some Lun success, some UECC, we search the success Lun to get the red pagetype to process. 
      However, success Lun may be the dummy data, so if there has the dummy data, we use its block type of dummy Red
      to decide next stage */
    PageType = L2_TB_Rebuild_TableGetPageTypeFromRED(ucSuperPuNum, &BlkType);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif        
    case PAGE_TYPE_DPBM:
    case PAGE_TYPE_PMTI:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            /* reset Page index and start to check next Page */
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
        }

        NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
    }
        break;
    case PAGE_TYPE_PMT:
    {
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
        break;
    case PAGE_TYPE_DUMMY:
    {
        /* this read last page case occurs dummy data, it means that it may be UECC + empty before */
        if (BlkType == BLOCK_TYPE_EMPTY)
        {
            DBG_Printf("SPU last page dummypage%d has empty blktype%d error\n", ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }

        if (BlkType == BLOCK_TYPE_AT0)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                /* reset Page index and start to check next Page */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
            }

            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else if (BlkType == BLOCK_TYPE_PMT)
        {
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
        else
        {
            DBG_Printf("[%s]SPU%d last page dummypage%d has error blktype%d\n", __FUNCTION__, ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableLastPageEmpty(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    /* use CurBlk index to search block type in RT */
    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        /* reset Page index and start to check next Page */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
        }

        NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
    }
        break;
    case PAGE_TYPE_PMT:
    {
        /* must be CurAT1Block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            /* reset Page index and start to check next Page */
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
        }

        pRT->m_RT[ucSuperPuNum].CurAT1PPO = INVALID_4F;
        NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableCheckNextPage(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlkType = BLOCK_TYPE_EMPTY;
    L2_TB_REBUILD_STATUS NextStatus;
    RED* pRed;

    /* next page all read success has two case :
        case 1: all success data is valid - > orginal path
        case 2: any read success Lun page is dummy page -> according to the block type of dummy page red, 
        select the next stage*/ 
    PageType =  L2_TB_Rebuild_TableGetPageTypeFromRED(ucSuperPuNum, &BlkType);

    /* read next page all success, need update table block TS */
    L2_TB_Rebuild_TableBlkSetTS(ucSuperPuNum);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif
    case PAGE_TYPE_DPBM:
    case PAGE_TYPE_PMTI:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    {
        /* locate latest PBIT for roll back erase count */
        if (PAGE_TYPE_PBIT == PageType)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TB_Rebuild_TableLocateLatestPBIT(ucSuperPuNum, pRed, ucLunInSuperPu);
            }
        }

        /* use Lun 0 page index to check whether current page is the last page */
        if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < (PG_PER_SLC_BLK - 1))
        {
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            /* last page - finish */
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }

        /* Because the super page TS in each Lun is the same, using Lun 0 Red TS to update max TS */
        pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, 0);
        L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, pRed);
    }
        break;
    case PAGE_TYPE_PMT:
    {
        /* go on reading */
        NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
    }
        break;
    case PAGE_TYPE_DUMMY:
    {
        /* next page has dummy data, it means that it may be UECC+empty or success+empty or success+UECC+empty before */
        if (BlkType == BLOCK_TYPE_EMPTY)
        {
            DBG_Printf("SPU last page dummypage%d has empty blktype%d error\n", ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }

        if ((BlkType == BLOCK_TYPE_AT0) || (BlkType == BLOCK_TYPE_PMT))
        {
            if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < (PG_PER_SLC_BLK - 1))
            {
                NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
            }
            else
            {
                /* last page - finish */
                NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
            }
        }
        else
        {
            DBG_Printf("[%s]SPU%d dummy pg type %d has error blktype%d\n", __FUNCTION__, ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableNextPageEmpty(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    U16  usBlock, usCurOpenBlk;
    U32 ulCurOpenBlkTS, ulOtherOpenBlkTS;
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        /* use Lun 0 block index to check whether there has more than 2 AT0 open block*/
        if ((pRT->m_RT[ucSuperPuNum].CurAT0Block[0] != INVALID_4F) && (pRT->m_RT[ucSuperPuNum].CurAT0PPO != 0))
        {
            /* there has more than 1 AT0 open block, need use TS to find which block is the correct open block */
			DBG_Printf("[%s]there are two open AT0 block, need check!!!\n", __FUNCTION__);
            usCurOpenBlk = pRT->m_RT[ucSuperPuNum].CurAT0Block[0]; // current Lun 0 target block
            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][0]; // current scaned Lun 0 AT0 block
            ulCurOpenBlkTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[0][usCurOpenBlk].LastPageTimeStamp; // current Lun 0 target block latest TS
            ulOtherOpenBlkTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[0][usBlock].LastPageTimeStamp; // current scaned Lun 0 AT0 block latest TS

            if (ulCurOpenBlkTS > ulOtherOpenBlkTS)
            {
                /* current scaned AT0 block may has abnormal shutdown during eraseing */
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                }
            }
            else if (ulCurOpenBlkTS < ulOtherOpenBlkTS)
            {
                /* current target AT0 block may has abnormal shutdown during eraseing */            
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu];
					L2_TB_Rebuild_RT_SetCurAT0Blk(ucSuperPuNum, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], ucLunInSuperPu);
                }
				/* use Lun 0 page index to updata AT0 PPO */
                L2_TB_Rebuild_RT_SetCurAT0PPO(ucSuperPuNum, pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
            }
            else
            {
                DBG_Printf("[%s]SPU%d CurAT0 target blk %d TS 0x%x = Cur AT0 open blk %d TS 0x%x error\n", __FUNCTION__, 
                    ucSuperPuNum, usCurOpenBlk, ulCurOpenBlkTS, usBlock, ulOtherOpenBlkTS);
                DBG_Getch();
            }

            NextStatus = L2_TB_REBUILD_ERROR_ERASE_ONE_BLOCK;
        }
        else
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                L2_TB_Rebuild_RT_SetCurAT0Blk(ucSuperPuNum, usBlock, ucLunInSuperPu);

                if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] <= 0 || 
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] > PG_PER_SLC_BLK)
                {
                    DBG_Printf("[%s]SPU%d CurLun%d AT0PPO%d ERROR!\n", __FUNCTION__, ucSuperPuNum, 
                        ucLunInSuperPu, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);

                    if (bTBRebuildEraseAll[ucSuperPuNum] == FALSE)
                    {
                        bTBRebuildEraseAll[ucSuperPuNum] = TRUE;
                    }
                }
                else
                {
                    /* use Lun 0 page index to updata AT0 PPO */
                    L2_TB_Rebuild_RT_SetCurAT0PPO(ucSuperPuNum, pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
                }
            }
			NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
        break;
    case PAGE_TYPE_PMT:
    {
        /* check whether there has exist open AT1 block */
		if ((pRT->m_RT[ucSuperPuNum].CurAT1Block[0] != INVALID_4F) && (pRT->m_RT[ucSuperPuNum].CurAT1PPO != INVALID_4F))
        {
            /* there has more than 1 AT1 open block, need use TS to find which block is the correct open block */
			DBG_Printf("[%s]there are two open AT1 block, need check!!!\n", __FUNCTION__);
            usCurOpenBlk = pRT->m_RT[ucSuperPuNum].CurAT1Block[0]; // current Lun 0 target block
			usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][0]; // current scaned Lun 0 AT1 block
			ulCurOpenBlkTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[0][usCurOpenBlk].LastPageTimeStamp; // current Lun 0 target block latest TS
			ulOtherOpenBlkTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[0][usBlock].LastPageTimeStamp; // current scaned Lun 0 AT1 block latest TS

            if (ulCurOpenBlkTS > ulOtherOpenBlkTS)
            {
                /* current scaned AT1 block may has abnormal shutdown during eraseing */
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                }
            }
            else if (ulCurOpenBlkTS < ulOtherOpenBlkTS)
            {
                /* current target AT1 block may has abnormal shutdown during eraseing */            
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu];
					pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                }

				/* use Lun 0 page index to updata AT1 PPO */
                pRT->m_RT[ucSuperPuNum].CurAT1PPO = pTBRebManagement->usPageIndex[ucSuperPuNum][0];
            }
            else
            {
                DBG_Printf("[%s]SPU%d CurAT1 target blk %d TS 0x%x = Cur AT0 open blk %d TS 0x%x error\n", __FUNCTION__, 
                    ucSuperPuNum, usCurOpenBlk, ulCurOpenBlkTS, usBlock, ulOtherOpenBlkTS);
                DBG_Getch();
            }

            NextStatus = L2_TB_REBUILD_ERROR_ERASE_ONE_BLOCK;
        }
        else
        {
            /* use Lun 0 page index to updata AT1 PPO */
            pRT->m_RT[ucSuperPuNum].CurAT1PPO = pTBRebManagement->usPageIndex[ucSuperPuNum][0];
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableNextPageAllUECC(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    /* use Lun 0 index to get the block and page type inforamtion in PBIT, which is recorded on reading first super page all success */
    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_PMT:
    {
        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] ++;
        if (pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] >= SLC_UECCCNT_ERASE_THRESHOLD)
        {
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu])
                {
                    pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = INVALID_4F;
                }

                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            }

            NextStatus = L2_TB_REBUILD_ERROR_ERASE_ONE_BLOCK;
            break;
        }

        /* use Lun 0 page index to check whether page index is the last page */
        if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < (PG_PER_SLC_BLK - 1))
        {
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            /* last page - finish */
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_TableNextPageNoEmpty(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlkType = BLOCK_TYPE_EMPTY;
    L2_TB_REBUILD_STATUS NextStatus;

    /* some Lun success, some UECC, we search the success Lun to get the red pagetype to process */
    PageType = L2_TB_Rebuild_TableGetPageTypeFromRED(ucSuperPuNum, &BlkType);

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif        
    case PAGE_TYPE_DPBM:
    case PAGE_TYPE_PMTI:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    case PAGE_TYPE_PMT:
    {
        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] ++;
        if (pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] >= SLC_UECCCNT_ERASE_THRESHOLD)
        {
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu])
                {
                    pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = INVALID_4F;
                }

                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            }

            NextStatus = L2_TB_REBUILD_ERROR_ERASE_ONE_BLOCK;
            break;
        }

        if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < (PG_PER_SLC_BLK - 1))
        {
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            /* last page - finish */
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
        break;
    case PAGE_TYPE_DUMMY:
    {
        /* this read next page case occurs dummy data, it means that it may be UECC+empty or success+empty or success+empty+UECC before */
        if (BlkType == BLOCK_TYPE_EMPTY)
        {
            DBG_Printf("SPU last page dummypage%d has empty blktype%d error\n", ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }

        if ((BlkType == BLOCK_TYPE_AT0) || (BlkType == BLOCK_TYPE_PMT))
        {
            if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < (PG_PER_SLC_BLK - 1))
            {
                NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
            }
            else
            {
                /* last page - finish */
                NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
            }
        }
        else
        {
            DBG_Printf("[%s]SPU%d dummypage%d has error blktype%d\n", __FUNCTION__, ucSuperPuNum, PageType, BlkType);
            DBG_Getch();
        }
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }

    return NextStatus;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableSetNextPageTargetInfor(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    U16  usBlock;
    PAGE_TYPE PageType;

    /* before dummy write, we should set next page to the target block PPO */
	/* if first page all UECC, next page 1 need dummy write. We cannot get page type from PBIT because current AT0 block
          doesn't have the valid page type (init page type is 0xf).*/
    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][0];
    //PageType = pPBIT[ucSuperPuNum]->m_PBIT_Info[0][usBlock].PageType;
    PageType = L2_TB_Rebuild_GetBlockType(ucSuperPuNum);
	
    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif        
    case PAGE_TYPE_DPBM:
    case PAGE_TYPE_PMTI:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    {
        /* mark current AT0 block and page */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] != INVALID_4F && pRT->m_RT[ucSuperPuNum].CurAT0PPO != 0)  //has more than one open block
            {
                DBG_Printf("[%s]SPU%d CurAT0Block%d ERROR!Too Many AT0 OpenBlk\n", __FUNCTION__, ucSuperPuNum, pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu]);
                if (bTBRebuildEraseAll[ucSuperPuNum] == FALSE)
                {
                    bTBRebuildEraseAll[ucSuperPuNum] = TRUE;
                }
            }
            else
            {
                L2_TB_Rebuild_RT_SetCurAT0Blk(ucSuperPuNum, usBlock, ucLunInSuperPu);
                if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] <= 0 || 
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] > PG_PER_SLC_BLK)
                {
                    DBG_Printf("[%s]SPU%d CurAT0PPO%d ERROR!\n", __FUNCTION__,ucSuperPuNum, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);
                    if (bTBRebuildEraseAll[ucSuperPuNum] == FALSE)
                    {
                        bTBRebuildEraseAll[ucSuperPuNum] = TRUE;
                    }
                }
                else
                {
                    L2_TB_Rebuild_RT_SetCurAT0PPO(ucSuperPuNum, (pTBRebManagement->usPageIndex[ucSuperPuNum][0] + 1));
                }
            }
        }
    }
        break;
    case PAGE_TYPE_PMT:
    {
        /* AT1 don't need to dummy, if current page is last page, do nothing.
        Otherwise, set next page is AT1 target block PPO */
        if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < PG_PER_SLC_BLK - 1)
        {
            /* use Lun 0 page index to updata AT1 PPO */
            pRT->m_RT[ucSuperPuNum].CurAT1PPO = pTBRebManagement->usPageIndex[ucSuperPuNum][0];
        }
    }
        break;
    default:
    {
        DBG_Printf("[%s]SPU%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, PageType);
        DBG_Getch();
    }
        break;
    }
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_TableUECCEraseCheckRT(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

    if ((ucSuperPuNum == RootTableAddr[0].m_PUSer) && (usBlock == RootTableAddr[0].m_BlockInPU))
    {
        U8 PgIndex;

        for (PgIndex = 0; PgIndex < RT_PAGE_COUNT; PgIndex++)
        {
            RootTableAddr[PgIndex].m_PPN = INVALID_8F;
        }
    }

    if (usBlock == pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu])
    {
        pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = INVALID_4F;
        pRT->m_RT[ucSuperPuNum].CurAT0PPO = INVALID_4F;
    }

    //##zoewen: even PBIT Page Cnt > 1 , we only use the first PG info when system performs table rebuild
    if (usBlock == pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][0])
    {
        pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][0] = INVALID_4F;
        pRT->m_RT[ucSuperPuNum].PBITPPO[0] = INVALID_4F;
    }

    if (usBlock == pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu])
    {
        pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = INVALID_4F;
        pRT->m_RT[ucSuperPuNum].CurAT1PPO = INVALID_4F;
    }

    return;
}

U32 MCU1_DRAM_TEXT L2_TB_Rebuild_TableBlockEraseAll(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;
    U32 ulRet;
    U8  ucFlashStatus;
    L2_TB_ERASEALL_STATUS *pStatus;

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBRebManagement->EraseAllStatus[ucSuperPuNum][ucLunInSuperPu]);

    switch (*pStatus)
    {
    case L2_TB_ERASEALL_START:
    {
        /* reset parameters */
        pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu] + 1;
        pTBRebManagement->usPageIndex[ucSuperPuNum][0] = 0;

        *pStatus = L2_TB_ERASEALL_ERASE_BLOCK;
    }
        break;

    case L2_TB_ERASEALL_ERASE_BLOCK:
    {
        /* erase current data block */
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            ;
        }
        else if (TRUE == pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError)
        {
            /* finish for bad block */
            *pStatus = L2_TB_ERASEALL_CURRENT_BLOCK_FINISH;
        }
        else
        {
            /* send read request to L3 */
            L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_TB_Rebuild_TableUECCEraseCheckRT(ucSuperPuNum, ucLunInSuperPu);// need double check

            L2_FtlEraseBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 
                &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, FALSE);
            *pStatus = L2_TB_ERASEALL_WAIT_ERASE_BLOCK;
        }
    }
        break;

    case L2_TB_ERASEALL_WAIT_ERASE_BLOCK:
    {
        /* wait for flash idle */
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
        {
            *pStatus = L2_TB_ERASEALL_CURRENT_BLOCK_FINISH;
        }
        else
        {
            DBG_Printf("L2_TB_Rebuild_TableBlockEraseAll ERASE_FAIL PU 0x%x BLK 0x%x add to BBT\n",
                ucSuperPuNum, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);

            L2_BbtAddBbtBadBlk(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), usBlock, RT_BAD_BLK, ERASE_ERR);

            gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;

            *pStatus = L2_TB_ERASEALL_ERASE_FAIL_ADDBBT;
        }
    }
        break;
    case L2_TB_ERASEALL_ERASE_FAIL_ADDBBT:
    {
        /* erase fail - add to BBT */
        if (TRUE == L2_BbtSave(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), INVALID_2F))
        {
            *pStatus = L2_TB_ERASEALL_CURRENT_BLOCK_FINISH;
        }
    }
        break;
    case L2_TB_ERASEALL_CURRENT_BLOCK_FINISH:
    {
        pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]++;
        if (pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] > g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu])
        {
            *pStatus = L2_TB_ERASEALL_SUCCESS;
        }
        else
        {
            *pStatus = L2_TB_ERASEALL_ERASE_BLOCK;
        }

        L2_PrintTBFullRecovery((U16)(*pStatus), ucSuperPuNum, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 
            pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
    }
        break;


    case L2_TB_ERASEALL_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_ERASEALL_SUCCESS:
    {
        /* init paramters */
        //L2_TB_Rebuild_MarkBadBlock(ucPuNum);
        //L2_RT_Rebuild_MarkTable(ucPuNum, 0);
        //pRT->m_RT[ucPuNum].ulMaxTimeStamp = 0;
        //bTBRebuildEraseAll[ucPuNum] = FALSE;
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("[%s]SPU%d Lun%d invalid Status 0x%x\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_TB_Rebuild_TableBlockErase(U8 ucSuperPuNum)
{
    U8  ucFlashStatus, ucLunInSuperPu;
    U16 usBlock;
    U32 ulRet, ulCmdRet;
    U8 ucMoveSuccess;
    static BOOL bNeedMoveBlk[SUBSYSTEM_SUPERPU_MAX];
    L2_TB_ERASE_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBRebManagement->TableBlkEraseStatus[ucSuperPuNum]);

    switch (*pStatus)
    {
    case L2_TB_ERASE_START:
    {
        /* if scanning first super page cause erase block handling, no matte AT0/AT1, erase directly */
        pTBRebManagement->ulLunEraseBitmap[ucSuperPuNum] = 0;
        bNeedMoveBlk[ucSuperPuNum] = FALSE;
        *pStatus = L2_TB_ERASE_BLOCK;
    }
        break;

    case L2_TB_ERASE_BLOCK:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                ;
            }
            else
            {
                /* current LUN can send command, mark bitmap corresponding LUN bit */
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                /* mark current send command Lun corresponding bit in erase bit map, it is used to check specific Lun erase status */
                pTBRebManagement->ulLunEraseBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                L2_FtlEraseBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu], 
                    &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, FALSE);
            }         
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_ERASE_WAIT;
        }
    }
        break;

    case L2_TB_ERASE_WAIT:
    {
        /* wait for flash idle */
        if (FALSE == L2_TB_Rebuild_WaitEraseFinish(ucSuperPuNum))
        {
            return ulRet;
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if ((pTBRebManagement->ulLunEraseBitmap[ucSuperPuNum] & (1<<ucLunInSuperPu)) == FALSE)
            {
                continue;
            }

            usBlock = pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu];
            ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
            {
                L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
                L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usBlock);
            }
            else
            {
                if (pTBRebManagement->bIsEraseFail[ucSuperPuNum][ucLunInSuperPu] != FALSE)
                {
                    /* after moving block, L2_TableErrorHandlingMoveBlock will add this erase fail block to BBT */
                    DBG_Printf("[%s] ERASE_FAIL SPU%d Lun%d BLK %d add to BBT\n", __FUNCTION__,
                        ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu]);
                }

                if (bNeedMoveBlk[ucSuperPuNum] == FALSE)
                {
                    bNeedMoveBlk[ucSuperPuNum] = TRUE;
                }
            }
        }

        if (bNeedMoveBlk[ucSuperPuNum] == FALSE)
        {
            *pStatus = L2_TB_ERASE_SUCCESS;
        }
        else
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                bIsUpdate[ucLunInSuperPu] = FALSE;
            }
            *pStatus = L2_TB_ERASE_FAIL_MOVEBLOCK;
        }
    }
        break;

    case L2_TB_ERASE_FAIL_MOVEBLOCK:
    {
        ucMoveSuccess = 0;
        /* erase fail - errorhandling move block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->bIsEraseFail[ucSuperPuNum][ucLunInSuperPu] == FALSE)
            {
                ucMoveSuccess++;
                continue;
            }

            ulCmdRet = L2_TableErrorHandlingMoveBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu], 0, ERASE_ERR);

            if (L2_ERR_HANDLING_PENDING == ulCmdRet)
            {
                ;
            }
            else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
            {
                if (bIsUpdate[ucLunInSuperPu] != TRUE)
                {
                    L2_RT_Rebuild_MarkTable(ucSuperPuNum, ucLunInSuperPu);
                    bIsUpdate[ucLunInSuperPu] = TRUE;
                }
                ucMoveSuccess++;
            }
            else
            {
                DBG_Printf("[%s] SPU%d Lun%d Erase ErrorHandling Fail\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                DBG_Getch();
            }
        }

        if (ucMoveSuccess == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_ERASE_SUCCESS;
        }
    }
        break;

    case L2_TB_ERASE_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_ERASE_SUCCESS:
    {
        /*reset erase infor variable*/
        L2_TB_Rebuild_TableEraseResetParam(ucSuperPuNum);
        *pStatus = L2_TB_ERASE_START;
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_Rebuild_TableBlockErase invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_RT_SecondPageSetParam(U8 ucSuperPuNum, U8* ucEmptyLun, U8* ucNonEmptyLun)
{
    U8 ucLunInSuperPu, ucFlashStatus, ucNonEmptyCnt = 0, ucEmptyCnt = 0;

    /* find which Lun second page is empty, which is non valid */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
        {
            continue;
        }

        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            *ucNonEmptyLun = ucLunInSuperPu;
            ucNonEmptyCnt++;
            DBG_Printf("[%s]SPU%d Lun %d 2nd pg success\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            if (*ucEmptyLun != INVALID_2F)
            {
                if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][*ucEmptyLun] == TRUE)
                {
                    if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                    {
                        /* special case: LunX first page UECC, next page empty, LunY first page UECC, next page empty
                                          -> erase directly, but health information will lost. */
                        DBG_Printf("special case: Lun%d 1st pg UECC(%d), 2nd pg empty, Lun%d 1st pg UECC(%d), 2nd pg empty\n", 
                            *ucEmptyLun, pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][*ucEmptyLun], 
                            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu], ucLunInSuperPu);
                        *ucEmptyLun = INVALID_2F;
                        *ucNonEmptyLun = INVALID_2F;
                    }
                    else
                    {
                        /* case 1: LunX first page UECC, next page empty, LunY first page success, next page empty
                                          -> LunX set to emptyLun to erase, LunY set to nonemptyLun */
                        DBG_Printf("Lun%d RT 1st pg UECC, 2nd pg empty\n", *ucEmptyLun);
                        DBG_Printf("But Lun%d RT 1st pg success, 2nd pg empty, set Lun %d to NonEmptyLun\n", 
                            ucLunInSuperPu, ucLunInSuperPu);
                        *ucNonEmptyLun = ucLunInSuperPu;
                    }

                    return;
                }
                else
                {
                    /* case 2: LunX first page success, next page empty, LunY first page UECC, next page empty
                                    -> LunY set to emptyLun to erase, LunX set to nonemptyLun */
                    DBG_Printf("Lun%d RT 1st pg success, 2nd pg empty\n", *ucEmptyLun);
                    DBG_Printf("But Lun%d RT 1st pg UECC, 2nd pg also empty, set Lun %d to NonEmptyLun\n", 
                        ucLunInSuperPu, *ucEmptyLun);
                    *ucNonEmptyLun = *ucEmptyLun;
                }
            }

            if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
            {
                /* abnormal shutdown during erasing RT block, erase not clean cause first page UECC and second page empty case */
                *ucEmptyLun = ucLunInSuperPu;
                DBG_Printf("[%s]SPU%d Lun %d 1st pg UECC 2nd pg empty\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
            }
            else
            {
                *ucEmptyLun = ucLunInSuperPu;
                DBG_Printf("[%s]SPU%d Lun %d 2nd pg empty\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
            }
        }
        else
        {		
            *ucNonEmptyLun = ucLunInSuperPu;
            ucNonEmptyCnt++;
            DBG_Printf("[%s]SPU%d Lun %d 2nd pg UECC\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
        }
    }

    /* only 2 Lun are not empty, double check it*/
    if (ucNonEmptyCnt > 2)
    {
        DBG_Printf("[%s]SPU %d %d Lun second page has data error!!\n", __FUNCTION__, ucSuperPuNum, ucNonEmptyCnt);
		DBG_Printf("[%s]Empty Lun %d, NonEmpty Lun %d, \n", __FUNCTION__, *ucEmptyLun, *ucNonEmptyLun);
        DBG_Getch();
    }
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_RT_CheckSecondPage(U8 ucSuperPuNum, U8 ucEmptyLun, U8 ucNonEmptyLun)
{
    U8 PgIndex, ucLunInSuperPu;
    RED *pRed;
    L2_TB_REBUILD_STATUS NextStatus;

    if ((ucEmptyLun == INVALID_2F) && (ucNonEmptyLun == INVALID_2F))
    {
        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = SUPERPU_LUN_NUM_BITMSK;
        for (ucLunInSuperPu= 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
            {
                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] =  pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucLunInSuperPu);
            }
        }

        return L2_TB_REBUILD_RT_ERASE_BLOCK;
    }

    if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucEmptyLun] == TRUE)
    {
        /* Lun's second page is empty and its first page is UECC, erase this Lun after rebuild RT finish */
        pTBRebManagement->usBlockErase[ucSuperPuNum][ucEmptyLun] =  pTBRebManagement->usBlockIndex[ucSuperPuNum][ucEmptyLun];

        /* decide next stage:
            case 1: LunX 1st page UECC, 2nd page empty; LunY 1st page UECC, 2nd page has data --> read LunY next page
            case 2: LunX 1st page UECC, 2nd page empty; LunY 1st page success, 2nd page has data --> read LunY last page */
        if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucNonEmptyLun] == TRUE)
        {
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucNonEmptyLun] = 0;
            NextStatus = L2_TB_REBUILD_RT_READ_NEXT_PAGE;
        }
        else
        {
            NextStatus = L2_TB_REBUILD_RT_READ_LAST_PAGE;
        }

        for (PgIndex = 0; PgIndex < RT_PAGE_COUNT; PgIndex++)
        {
            RootTableAddr[PgIndex].m_OffsetInSuperPage = ucNonEmptyLun;
            RootTableAddr[PgIndex].m_BlockInPU =  pTBRebManagement->usBlockIndex[ucSuperPuNum][ucNonEmptyLun];
        }

        /* set non empty Lun corresponding bit to 0 to be used in read lase or next page */
        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = SUPERPU_LUN_NUM_BITMSK;
        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucNonEmptyLun);

        /* reset first page UECC variable */
        //pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucEmptyLun] = FALSE;
        //pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucNonEmptyLun] = FALSE;
    }
    else
    {
        /* Lun's 2nd page is empty and 1st page is success, erase the other Lun after rebuild RT finish */
        /* Now we find the latest RT page location, set RootTableAddr variable for load latest RT information */
        pTBRebManagement->usBlockErase[ucSuperPuNum][ucNonEmptyLun] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucNonEmptyLun];
        for (PgIndex = 0; PgIndex < RT_PAGE_COUNT; PgIndex++)
        {
            RootTableAddr[PgIndex].m_OffsetInSuperPage = ucEmptyLun;
            RootTableAddr[PgIndex].m_BlockInPU =  pTBRebManagement->usBlockIndex[ucSuperPuNum][ucEmptyLun];
        }

        /* update latest TS */
        pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucNonEmptyLun);
        L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, pRed);
        
        NextStatus = L2_TB_REBUILD_RT_READ_HEALTH_INFOR;
    }

    for (PgIndex = 0; PgIndex < RT_PAGE_COUNT; PgIndex++)
    {
        RootTableAddr[PgIndex].m_PUSer = ucSuperPuNum;
        RootTableAddr[PgIndex].m_PageInBlock = 0;
        RootTableAddr[PgIndex].m_LPNInPage = 0;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_CheckRTFirstSuperPage(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucCurRTLun[2], ucRdSucCnt = 0;
    U8 PgIndex, ucFinalRTLun;
    U16 ucFinalRTBlk;
    U32 ulFirstPageTS[2];
    RED* pRed;
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    /* enter this function, there are only two case:
        1. 1 Lun read success: get read success Lun and set bitmap to read this Lun last page 
        2. 2 Lun read success: compare red TS, select biggest TS and set erase bitmap to erase the other Lun */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (pTBRebManagement->usReadSuccessLun[ucSuperPuNum][ucLunInSuperPu] == TRUE)
        {
            ucCurRTLun[ucRdSucCnt] = ucLunInSuperPu;
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            PageType = pRed->m_RedComm.bcPageType;

            if (PageType != PAGE_TYPE_RT)
            {
                DBG_Printf("[%s]SPU%d Lun%d Pagetype%d not RT\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, PageType);
                DBG_Getch();
            }

            ulFirstPageTS[ucRdSucCnt] = pRed->m_RedComm.ulTimeStamp;
            ucRdSucCnt++;
        }
    }

#ifdef SIM
    if (ucRdSucCnt > 2)
    {
        DBG_Printf("[%s]:SPU%d %d Lun RT first page read success\n", __FUNCTION__, ucSuperPuNum, ucRdSucCnt);
        DBG_Getch();
    }
#endif

    if (ucRdSucCnt == 1)
    {
        ucFinalRTLun = ucCurRTLun[0];
        ucFinalRTBlk = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucFinalRTLun];
        //ucFinalRTLun = ucCurRTLun[0];
        NextStatus = L2_TB_REBUILD_RT_READ_LAST_PAGE;
        /* set read success Lun correspongind bit to 0 to be use in read last page*/
        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = SUPERPU_LUN_NUM_BITMSK;
        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucFinalRTLun);
    }
    else  // 2 lun read success, compare TS
    {
        /* Need to set erased block, but erase the block after cimpleting to rebuild RT */
        DBG_Printf("[%s]SPU%d 2 Lun has valid RT page\n", __FUNCTION__, ucSuperPuNum);
        if (ulFirstPageTS[0] > ulFirstPageTS[1] )
        {
            ucFinalRTLun = ucCurRTLun[0];
            ucFinalRTBlk = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucFinalRTLun];
            //ucFinalRTLun = ucCurRTLun[0];
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucCurRTLun[1]] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucCurRTLun[1]];
        }
        else if (ulFirstPageTS[0] < ulFirstPageTS[1] )
        {
            ucFinalRTLun = ucCurRTLun[1];
            ucFinalRTBlk =pTBRebManagement->usBlockIndex[ucSuperPuNum][ucFinalRTLun];
            //ucFinalRTLun = ucCurRTLun[1];
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucCurRTLun[0]] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucCurRTLun[0]];
        }
#ifdef SIM
        else
        {
            DBG_Printf("[%s]:SPU%d 2 first page TS are the same 0x%x and 0x%x\n", __FUNCTION__, ucSuperPuNum, ulFirstPageTS[0] , ulFirstPageTS[1]);
            DBG_Getch();
        }
#endif
        RootTableAddr[0].m_PageInBlock = 0;
        NextStatus = L2_TB_REBUILD_RT_READ_HEALTH_INFOR;
    }

    /* we find current RT block, set RT location information */
    for (PgIndex = 0; PgIndex < RT_PAGE_COUNT; PgIndex++)
    {
        RootTableAddr[PgIndex].m_PUSer = ucSuperPuNum;
        RootTableAddr[PgIndex].m_OffsetInSuperPage = ucFinalRTLun;
        RootTableAddr[PgIndex].m_BlockInPU = ucFinalRTBlk;
        RootTableAddr[PgIndex].m_PageInBlock = 0;
        RootTableAddr[PgIndex].m_LPNInPage = 0;
    }

    pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucFinalRTLun);
    L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, pRed);
    
    return NextStatus;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_RT_FirstSuperPageUECCSetParam(U8 ucSuperPuNum, U8* ucUeccCnt, U8* ucEmptyCnt, U8* ucSucCnt)
{
    U8 ucLunInSuperPu, ucFlashStatus;
    RED* pRed;

    *ucUeccCnt = pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum];
    *ucEmptyCnt =pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum];
    *ucSucCnt = pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum];

    /* init bitmap to INVLID_4F, and set UECC or success Lun corrponding bit to 0 to read second/next page use. 
        Then if Lun read UECC, set first page UECC flag */
    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = SUPERPU_LUN_NUM_BITMSK;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            if (pRed->m_RedComm.bcPageType != PAGE_TYPE_RT)
            {
                DBG_Printf("[%s]SPU%d Lun%d RT PageType%d invalid\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, pRed->m_RedComm.bcPageType);
                DBG_Getch();
            }

            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucLunInSuperPu);	 
			DBG_Printf("[%s]Lun%d 1st pg success\n", __FUNCTION__, ucLunInSuperPu);
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            continue;
        }
        else
        {
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucLunInSuperPu);	
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = TRUE;
			DBG_Printf("[%s]Lun%d 1st pg UECC\n", __FUNCTION__, ucLunInSuperPu);
        }
    }

    /* backup bitmap */
    pTBRebManagement->ulTempLunBitmap[ucSuperPuNum] = pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum];
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_CheckRTFirstSuperPageUECC(U8 ucSuperPuNum)
{
    U8 ucUeccCnt, ucEmptyCnt, ucSucCnt;
    L2_TB_REBUILD_STATUS NextStatus;

    /* enter this function, there are three case:
           1. 1 Lun UECC, others empty: read next page to find the latest RT location
           2. 1 Lun UECC, 1 Lun success: both Lun need to read second page to check which Lun is open and which is closed
           3. 2 Lun UECC: both Lun need to read second page to ckeck which Lun is open and which is closed */
    
    L2_TB_Rebuild_RT_FirstSuperPageUECCSetParam(ucSuperPuNum, &ucUeccCnt, &ucEmptyCnt, &ucSucCnt);
    DBG_Printf("[%s]SPU%d 2 Lun has RT page, but has UECC(cnt:%d)\n", __FUNCTION__, ucSuperPuNum, ucUeccCnt);
    if ((ucUeccCnt == 1) && (ucEmptyCnt == LUN_NUM_PER_SUPERPU -1))
    {
        /* 1 Lun Uecc, others empty */
        //NextStatus = L2_TB_REBUILD_RT_READ_NEXT_PAGE;
        DBG_Printf("[%s]read last page\n", __FUNCTION__);
        NextStatus = L2_TB_REBUILD_RT_READ_LAST_PAGE;
    }
    else if (((ucUeccCnt == 2) || ((ucUeccCnt == 1) && (ucSucCnt== 1))) && (LUN_NUM_PER_SUPERPU -2))
    {
        /* case 1: 1 Lun Uecc, 1 Lun success, others empty
                 case 2: 2 Lun Uecc, other emptys */
        DBG_Printf("[%s]read second page\n", __FUNCTION__);
        NextStatus = L2_TB_REBUILD_RT_READ_SECOND_PAGE;
    }
    else
    {
        DBG_Printf("[%s]SPU%d RT first page no match UECC case, LunSucCnt%d, LunEmptyCnt%d, LunUECCCnt%d\n", 
            __FUNCTION__, ucSuperPuNum, ucSucCnt, ucEmptyCnt, ucUeccCnt);
        DBG_Getch();
    }

    return NextStatus;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_RT_IsNeedErase(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    BOOL bIsNeedErase = FALSE;

    /* init RT block erase stage and parameter */
    pTBRebManagement->TableBlkEraseStatus[ucSuperPuNum] = L2_TB_ERASE_START;
    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = SUPERPU_LUN_NUM_BITMSK;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] != INVALID_4F)
        {
            /* Lun corresponding bit = 1: not need to erase, bit = 0: need erase */
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucLunInSuperPu);
            bIsNeedErase = TRUE;
			DBG_Printf("[%s]SPU%d Lun %d blk %d RT need erase\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu]);
        }
    }
 
    return bIsNeedErase;
}

/* Use this function to get the LunX when only need to read or check 1 Lun.
    The following 4 state machine will call this function because they are only read or check 1 Lun:
    1. Read last page; 2. check last page; 3. read next page; 4. check next page */
U8 MCU1_DRAM_TEXT L2_TB_Rebuild_RT_GetCurReadLun(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
        {
            continue;
        }
        break;
    }

    return ucLunInSuperPu;
}

U32 MCU1_DRAM_TEXT L2_TB_Rebuild_RT(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucFlashStatus;
    U8 ucEmptyLun = INVALID_2F, ucNonEmptyLun = INVALID_2F;
    static U16 ucRTpageNum[SUBSYSTEM_SUPERPU_MAX];
    static U16 ucReadFromLastPage[SUBSYSTEM_SUPERPU_MAX];
    static U16 usCurPPO[SUBSYSTEM_SUPERPU_MAX];
    U16 usPageIndex;
    U32 ulRet, ulBufferAddr;
    RED *pRed;
    L2_TB_REBUILD_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBRebManagement->RebuildStatus[ucSuperPuNum]);

    switch (*pStatus)
    {
        case L2_TB_REBUILD_START:
        {
            /* mark bad block */
            L2_TB_Rebuild_MarkBadBlock(ucSuperPuNum);

            /* mark all Table Block */
            L2_TB_Rebuild_MarkAllTable(ucSuperPuNum);

            /* if RT has been load success in normal boot path( L2_Load_RT), we don't rebuild RT again 
                  because the latest RT location and health information exsit */
            if (bRTHasLoad[ucSuperPuNum] == TRUE)
            {
                L2_TB_Rebuild_TableUpdateBlockTS(ucSuperPuNum);
                *pStatus = L2_TB_REBUILD_RT_FINISH;
            }
            else
            {
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu];
                }

                ucRTpageNum[ucSuperPuNum] = 0;
                ucReadFromLastPage[ucSuperPuNum] = FALSE;
                /* this variable is used to record the target RT block PPO */
                usCurPPO[ucSuperPuNum] = 0;
                *pStatus = L2_TB_REBUILD_RT_READ_FIRST_SUPER_PAGE;
            }
        }
            break;
        case L2_TB_REBUILD_RT_READ_FIRST_SUPER_PAGE:
        {
            /* read 1st page redundant of each block */
            gulMaxTableBlockTS[ucSuperPuNum] = 0;
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
                {
                    continue;
                }
 
                if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                {
                    ;
                }
                else
                {
                    /*current LUN can send command, mark bitmap corresponding LUN bit*/
                    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                    /* update page index to 1st page */
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;

                    /* get flashaddr, temp red and send read request to L3 */
                    FlashAddr.m_PUSer = ucSuperPuNum;
                    FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                    FlashAddr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                    FlashAddr.m_PageInBlock = 0;
                    FlashAddr.m_LPNInPage = 0;

                    pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                    L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);
                }
            }

            /*wait all LUN send command*/
            if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
            {
                *pStatus = L2_TB_REBUILD_RT_CHECK_FIRST_SUPER_PAGE;
            }
        }
            break;
        case L2_TB_REBUILD_RT_CHECK_FIRST_SUPER_PAGE:
        {
            /* wait for flash idle */
            if (FALSE== L2_TB_Rebuild_WaitAllFlashFinish(ucSuperPuNum, pStatus))
            {
                return ulRet;
            }

            if ((pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] <= 2) &&
                (pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] != LUN_NUM_PER_SUPERPU) &&
                (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] == 0))
            {
                /* 1 or 2 Lun read success, others are empty */
                *pStatus =L2_TB_Rebuild_CheckRTFirstSuperPage(ucSuperPuNum);
            }
            else if (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] != 0)
            {
                /* 1 or 2 Lun first page read UECC */
                *pStatus = L2_TB_Rebuild_CheckRTFirstSuperPageUECC(ucSuperPuNum);
            }
            else if (pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
            {
                /* all first page empty, current SuperPu no valid RT */
                *pStatus = L2_TB_REBUILD_RT_FINISH;
            }
            else
            {
                DBG_Printf("[%s]SPU%d RT first superpage no match case, SucCnt%d, EmptyCnt%d, UECCCnt%d\n", __FUNCTION__, ucSuperPuNum,
                     pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum], pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum], 
                     pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum]);
                DBG_Getch();
            }
        }
            break;
        case L2_TB_REBUILD_RT_READ_LAST_PAGE:
        {
            /* only 1 LUN first page read success or UECC case will enter this case to read last page*/
            ucLunInSuperPu = L2_TB_Rebuild_RT_GetCurReadLun(ucSuperPuNum);

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                /* update page index to last page */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = (PG_PER_SLC_BLK - 1);

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_PageInBlock = (PG_PER_SLC_BLK - 1);
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);

                /* don't set Lun corresponding bit to 1 because check last page can use this bit to know which Lun needs to be checked */
                *pStatus = L2_TB_REBUILD_RT_CHECK_LAST_PAGE;
            }
        }
            break;
        case L2_TB_REBUILD_RT_CHECK_LAST_PAGE:
        {
            ucLunInSuperPu = L2_TB_Rebuild_RT_GetCurReadLun(ucSuperPuNum);
            ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                ;
            }
            else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, pRed);
                usCurPPO[ucSuperPuNum] = (PG_PER_SLC_BLK - 1);
                if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                {
                    /* first page UECC, not RT location information. Now read last page success, we need to set RootTableAddr */
                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PUSer = ucSuperPuNum;
                        RootTableAddr[usPageIndex].m_OffsetInSuperPage = ucLunInSuperPu;
                        RootTableAddr[usPageIndex].m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                        RootTableAddr[usPageIndex].m_PageInBlock = usCurPPO[ucSuperPuNum] - usPageIndex;
                        RootTableAddr[usPageIndex].m_LPNInPage = 0;
                    }
                }
                else
                {
                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PageInBlock = usCurPPO[ucSuperPuNum] - usPageIndex;
                    }
                }

                *pStatus = L2_TB_REBUILD_RT_READ_HEALTH_INFOR;
            }
            else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
            {
                if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                {
                    /* first page UECC, no record RT information, so we set some RT location in last page empty because 
                               current RT block is target block and read next page to find the valid RT page */
                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PUSer = ucSuperPuNum;
                        RootTableAddr[usPageIndex].m_OffsetInSuperPage = ucLunInSuperPu;
                        RootTableAddr[usPageIndex].m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                        RootTableAddr[usPageIndex].m_LPNInPage = 0;
                    }
                }

                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
                *pStatus = L2_TB_REBUILD_RT_READ_NEXT_PAGE;
            }
            else
            {
                /* read last page UECC, we can read from latest page - 1 to speedup */
                ucReadFromLastPage[ucSuperPuNum] = TRUE;
                usCurPPO[ucSuperPuNum] = (PG_PER_SLC_BLK - 1);
                if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                {
                    /* first page UECC, no record RT information, so we set some RT location in last page UECC because 
                               current RT block is target block and read next page to find the valid RT page */
                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PUSer = ucSuperPuNum;
                        RootTableAddr[usPageIndex].m_OffsetInSuperPage = ucLunInSuperPu;
                        RootTableAddr[usPageIndex].m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                        RootTableAddr[usPageIndex].m_LPNInPage = 0;
                    }
                }
                else
                {
                    /* first page read success, don't set RT superpu, lun, block inforamtion here, just go to read next page from last page - 1 */
                    /* do nothing now */
                }

                *pStatus = L2_TB_REBUILD_RT_READ_NEXT_PAGE;
            }
        }
            break;
        case L2_TB_REBUILD_RT_READ_SECOND_PAGE:
        {
            /* 2 LUN read first page cause success and UECC combindation (2 RT block has RT data) */
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
               if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
                {
                    continue;
                } 
 
                /* read next page redundant of each block */
                if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                {
                    /*current LUN can send command, mark bitmap corresponding LUN bit*/
                    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                    /* update page index to next page */
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 1;

                    /* get flashaddr, temp red and send read request to L3 */
                    FlashAddr.m_PUSer = ucSuperPuNum;
                    FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                    FlashAddr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                    FlashAddr.m_PageInBlock = 1;
                    FlashAddr.m_LPNInPage = 0;

                    pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                    L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);
                }
            }

            if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
            {
                /* use temp bitmap to recovery original load Lun bitmap and use in check second page stage*/
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = pTBRebManagement->ulTempLunBitmap[ucSuperPuNum];
                *pStatus = L2_TB_REBUILD_RT_CHECK_SECOND_PAGE;
            }
        }
            break;
        case L2_TB_REBUILD_RT_CHECK_SECOND_PAGE:
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
                {
                    continue;
                }

                if (SUBSYSTEM_STATUS_PENDING == pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu])
                {
                    /* For read second page 2 Lun, any Lun flash status isn't finish, return directly  */
                    return L2_TB_WAIT;
                }
            }

            L2_TB_Rebuild_RT_SecondPageSetParam(ucSuperPuNum, &ucEmptyLun, &ucNonEmptyLun);
	        DBG_Printf("[%s]RD 2nd pg: empty Lun %d, NonEmpty Lun %d\n",__FUNCTION__, ucEmptyLun, ucNonEmptyLun);
            *pStatus = L2_TB_Rebuild_RT_CheckSecondPage(ucSuperPuNum, ucEmptyLun, ucNonEmptyLun);
        }
            break;
        case L2_TB_REBUILD_RT_READ_NEXT_PAGE:
        {
            /* only 1 LUN read first page success and last page empty will enter this stage(1 RT block has RT data)*/
            ucLunInSuperPu = L2_TB_Rebuild_RT_GetCurReadLun(ucSuperPuNum);
 
            /* read next page redundant of each block */
            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                /* update page index to next page or previous page*/
                if (ucReadFromLastPage[ucSuperPuNum] == FALSE)
                {
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]++;
                }
                else
                {
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]--;
                }

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);

                /* don't set Lun corresponding bitmap to 1 because it can be used in RT check next page*/
                *pStatus = L2_TB_REBUILD_RT_CHECK_NEXT_PAGE;
            }
        }
            break;
        case L2_TB_REBUILD_RT_CHECK_NEXT_PAGE:
        {
            ucLunInSuperPu = L2_TB_Rebuild_RT_GetCurReadLun(ucSuperPuNum);
            ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                ;
            }
            else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)	
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TB_Rebuild_TableUpdateMaxTS(ucSuperPuNum, pRed);
                if (ucReadFromLastPage[ucSuperPuNum] == FALSE)
                {
                    if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                    {
                        /* now we has the valid RT page, but need to read next page to find the newer RT page */
                        pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
                    }

                    /* record current page, it may be the target PPO*/
                    usCurPPO[ucSuperPuNum] = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                    *pStatus = L2_TB_REBUILD_RT_READ_NEXT_PAGE;
                }
                else
                {
                    *pStatus = L2_TB_REBUILD_RT_READ_HEALTH_INFOR;
                }

                /* record current latest read success RT page number */
                for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                {
                    RootTableAddr[usPageIndex].m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                }
            }
            else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
            {
                if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                {
                    /* all previous RT pages are UECC, no RT information, set previous RT page is PPO and rebuild RT finish */
					pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PageInBlock = usCurPPO[ucSuperPuNum];
                    }
                    *pStatus = L2_TB_REBUILD_RT_FINISH;
                }
                else
                {
                    /* use previous record latest valid RT page */
                    *pStatus = L2_TB_REBUILD_RT_READ_HEALTH_INFOR;              
                }
            }
            else
            {
                /* read next page UECC */
                if (ucReadFromLastPage[ucSuperPuNum] == FALSE)
                {
                    usCurPPO[ucSuperPuNum] = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                    if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] < (PG_PER_SLC_BLK - 1))
                    {
                        *pStatus = L2_TB_REBUILD_RT_READ_NEXT_PAGE;
                    }
                    else
                    {
                        if (pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                        {
                            /* RT last page, but all previous RT pages are UECC, no RT information, set previous RT page is PPO and rebuild RT finish */
		                    pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
                            for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                            {
                                RootTableAddr[usPageIndex].m_PageInBlock = usCurPPO[ucSuperPuNum];
                            }

                            if (TRUE == L2_TB_Rebuild_RT_IsNeedErase(ucSuperPuNum))
                            {
                                *pStatus = L2_TB_REBUILD_RT_ERASE_BLOCK;
                            }
                            else
                            {
                                *pStatus = L2_TB_REBUILD_RT_FINISH;
                            }
                        }
                        else
                        {
                            /* RT last page and has valid RT, go to read RT heathle */
                            *pStatus = L2_TB_REBUILD_RT_READ_HEALTH_INFOR; 
                        }
                    }
                }
                else
                {
                    /* If page index is 0, it indicates that there are no valid RT.
                               Otherwise, read from next page - 1 */
                    if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] == 0)
                    {
                        *pStatus = L2_TB_REBUILD_RT_FINISH;
                    }
                    else
                   	{
                   	    *pStatus = L2_TB_REBUILD_RT_READ_NEXT_PAGE;
                   	}
                }
            }
        }
            break;
        case L2_TB_REBUILD_RT_READ_HEALTH_INFOR:
        {
            ucLunInSuperPu = RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_OffsetInSuperPage;
            if (TRUE ==  L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

                FlashAddr.m_PUSer = RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_PUSer;
                FlashAddr.m_OffsetInSuperPage = RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_OffsetInSuperPage;
                FlashAddr.m_BlockInPU = RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_BlockInPU;
                FlashAddr.m_PageInBlock = RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_PageInBlock;
                FlashAddr.m_LPNInPage = 0;

                DBG_Printf("[%s]SPU%d read latest RT infor Lun%d blk%d pg%d\n", __FUNCTION__, FlashAddr.m_PUSer, FlashAddr.m_OffsetInSuperPage,
					FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);
                //ucLunInSuperPu =RootTableAddr[ucRTpageNum].m_OffsetInSuperPage;
                ulBufferAddr = COM_GetMemAddrByBufferID((L2RTStartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
                L2_FtlReadLocal((U32 *)ulBufferAddr, &FlashAddr, &(pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                    (U32 *)&(pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu]), LPN_PER_BUF, 0, TRUE, TRUE);

                *pStatus = L2_TB_REBUILD_RT_CHECK_READ_HEALTH_INFOR;
            }
        }
            break;
        case L2_TB_REBUILD_RT_CHECK_READ_HEALTH_INFOR:
        {
            /* wait flash idle and copy data to pRT */
            ucLunInSuperPu = RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_OffsetInSuperPage;
            ucFlashStatus = pRTManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                ;
            }
            else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                U32 ulDataAddr;

                ulBufferAddr = COM_GetMemAddrByBufferID((L2RTStartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);
                ulDataAddr = (U32)pRT + ucRTpageNum[ucSuperPuNum] * BUF_SIZE;
                COM_MemCpy((U32 *)ulDataAddr, (U32 *)ulBufferAddr, BUF_SIZE / sizeof(U32));

                ucRTpageNum[ucSuperPuNum]++;
                if (ucRTpageNum[ucSuperPuNum] >= RT_PAGE_COUNT)
                {
                    if (TRUE == L2_TB_Rebuild_RT_IsNeedErase(ucSuperPuNum))
                    {
                        *pStatus = L2_TB_REBUILD_RT_ERASE_BLOCK;
                    }
                    else
                    {
                        *pStatus = L2_TB_REBUILD_RT_FINISH;
                    }

                    for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                    {
                        RootTableAddr[usPageIndex].m_PageInBlock = usCurPPO[ucSuperPuNum] - usPageIndex;
                    }
                }
                else
                {
                    *pStatus = L2_TB_REBUILD_RT_READ_HEALTH_INFOR;
                }
            }
            else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
            {
                /*  Load RT Page 2nd time EMPTY_PG: must be function error */
                DBG_Printf("[%s] PU 0x%x LUN 0x%x WAIT_FINISH EMPTY_PG\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                DBG_Printf("[%s] RTpageNum%d Blk 0x%x page 0x%x\n", __FUNCTION__, ucRTpageNum[ucSuperPuNum], RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_BlockInPU, RootTableAddr[ucRTpageNum[ucSuperPuNum]].m_PageInBlock);
                DBG_Getch();
            }
            else
            {
                /* target RT page RED read success, but finial read page UECC, current do nothing, this is open issue */
                DBG_Printf("[%s]SPU%d LUN%d Read target Red success, but data UECC\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                if (TRUE == L2_TB_Rebuild_RT_IsNeedErase(ucSuperPuNum))
                {
                    *pStatus = L2_TB_REBUILD_RT_ERASE_BLOCK;
                }
                else
                {
                    *pStatus = L2_TB_REBUILD_RT_FINISH;
                }

                for (usPageIndex = 0; usPageIndex < RT_PAGE_COUNT; usPageIndex++)
                {
                    RootTableAddr[usPageIndex].m_PageInBlock = usCurPPO[ucSuperPuNum] - usPageIndex;
                }
            }
        }
            break;
        case L2_TB_REBUILD_RT_ERASE_BLOCK:
        {
            if (L2_TB_SUCCESS == L2_TB_Rebuild_TableBlockErase(ucSuperPuNum))
            {
                *pStatus = L2_TB_REBUILD_RT_FINISH;
            }
        }
            break;
        case L2_TB_REBUILD_RT_FINISH:
        {
            if (bRTHasLoad[ucSuperPuNum] != TRUE)
            {
                L2_TB_Rebuild_TableUpdateBlockTS(ucSuperPuNum);
            
                L2_RTResumeSysInfo();
                g_pSubSystemDevParamPage->PowerCycleCnt++;
            }

            g_pSubSystemDevParamPage->SYSUnSafeShutdownCnt++;
            gbGlobalInfoSaveFlag = TRUE;
            /* Both L2_Load_RT success and find latest RT in current function, we need to reset AT0/AT1 table location variable in RT.
                  Because AT0/AT1 table information will be rebuild later */
            L2_RT_Init_AT0AT1Infor();
            L2_RT_Rebuild_ResetParam(ucSuperPuNum);
            ulRet = L2_TB_SUCCESS;
        }
            break;
        default:
        {
            DBG_Printf("L2_TB_Rebuild_TableBlockErase invalid Status 0x%x\n", *pStatus);
            DBG_Getch();
        }
            break;
    }
    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_TB_Rebuild_TableBlock(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucEraseSucCnt, i;
    static U8 dummyWriteCnt[SUBSYSTEM_SUPERPU_MAX];
    U16 usBlock, usPPO;
    U32 ulRet, ulBufferAddr;
    RED *pRed;
    RED DummyWriteSpare;
    L2_TB_REBUILD_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBRebManagement->RebuildStatus[ucSuperPuNum]);

    switch (*pStatus)
    {
    case L2_TB_REBUILD_START:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = pRTManagement->usRTBlock[ucSuperPuNum][ucLunInSuperPu] + 1;
            /* reset mark table block finish flag, which will be used when dummy write fail move block complete*/
            bIsUpdate[ucLunInSuperPu] = FALSE;

            /* reset PBIT red information for recording latest PBIT */
            pRed = &(pTBRedInfo->PBIT_Red[ucSuperPuNum][ucLunInSuperPu]);
            HAL_DMAESetValue((U32)pRed, (RED_SW_SZ_DW * DWORD_SIZE), 0);
        }

        bTBRebuildEraseAll[ucSuperPuNum] = FALSE;
        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
        bHandleSPORDuringErase[ucSuperPuNum] = FALSE;
        /*use Lun 0 index to represent table super page UECC count */
        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] = 0;
        FirstPageEmptyAT1Index[ucSuperPuNum] = INVALID_2F;
        AT1PartialWritePage[ucSuperPuNum] = INVALID_4F;
        dummyWriteCnt[ucSuperPuNum] = 0;
        *pStatus = L2_TB_REBUILD_READ_FIRST_PAGE;
    }
        //break;

    case L2_TB_REBUILD_READ_FIRST_PAGE:
    {
        /* read 1st page redundant of each block */
        gulMaxTableBlockTS[ucSuperPuNum] = 0;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                ;
            }
            else if (TRUE == pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError)
            {
                /* finish for bad block */
                pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]++;
            }
            else if (TRUE != pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bTable)
            {
                /* not Table block, Error! */
                DBG_Printf("[%s]1st page SPU%d LUN%d invalid Block%d\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, usBlock);
                DBG_Getch();
            }
            else
            {
                /*current LUN can send command, mark bitmap corresponding LUN bit*/
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                /* set page index to 0 */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = 0;
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);
            }
       	}

	    /*wait all LUN send command*/
        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_REBUILD_CHECK_FIRST_PAGE;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_FIRST_PAGE:
    {
        /* wait for flash idle */
        if (FALSE== L2_TB_Rebuild_WaitAllFlashFinish(ucSuperPuNum, pStatus))
        {
            return ulRet;
        }

        if (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_Rebuild_TableCheckFirstPage(ucSuperPuNum);
        }
        else if (pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            /* all LUN read empty*/
            U8 AT0BlkO;

            for(ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] == INVALID_4F)
                {
                    for (AT0BlkO = 0; AT0BlkO < AT0_BLOCK_COUNT; AT0BlkO++)
                    {
                        if (pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][AT0BlkO] == pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu])
                        {
                            L2_TB_Rebuild_RT_SetCurAT0Blk(ucSuperPuNum, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], ucLunInSuperPu);
                            L2_TB_Rebuild_RT_SetCurAT0PPO(ucSuperPuNum, 0);
                            break;
                        }
                    }
                }
            }

            /* use Lun 0 index to find current AT1 index which first page is empty */
            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][0];
            if ((usBlock >= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][0])
                && (usBlock <= pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][AT1_BLOCK_COUNT - 1]))
            {                
                if (FirstPageEmptyAT1Index[ucSuperPuNum] == INVALID_2F)
                {
                    for (i = 0; i < AT1_BLOCK_COUNT; i++)
                    {
                        if (pRT->m_RT[ucSuperPuNum].AT1BlockAddr[0][i] == usBlock)
                        {
                            FirstPageEmptyAT1Index[ucSuperPuNum] = i;
                            DBG_Printf("Find 1st free AT1 block %d, index %d\n", usBlock, i);
                            break;
                        }
                    }
                }
            }
            
            *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
        else if (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_Rebuild_TableFirstPageAllUECC(ucSuperPuNum);
        }
        else if ((pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] == 0)
                && (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] != 0) 
                && (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] != 0))
        {
            *pStatus = L2_TB_Rebuild_TableFirstPageSuccessandUECC(ucSuperPuNum);
        }
        else
        {
            /* others case:
                       1. read Lun flash status = success + UECC + empty
                       2. read Lun flash status = success + empty
                       3. read Lun flash status = UECC + empty */
            if (L2_TB_Rebuild_NeedEraseBlock(ucSuperPuNum) == TRUE)
            {
                *pStatus = L2_TB_Rebuild_TablePrepareEraseBitmap(ucSuperPuNum);
            }
            else
            {
                *pStatus = L2_TB_Rebuild_TableOthersCaseNextStage(ucSuperPuNum);
            }
        }

        /* reset Lun load bitmap and flash read infor variable. But if next stage is erase, don't reset because LoadLunbitmap indicate
            which Lun need to erase */
        if (*pStatus != L2_TB_REBUILD_ERROR_ERASE_ONE_BLOCK)
        {
            L2_TB_Rebuild_TableResetParam(ucSuperPuNum); 
        }
    }
        break;

    case L2_TB_REBUILD_READ_LAST_PAGE:
    {
        /* read last page redundant of each block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                /*current LUN can send command, mark bitmap corresponding LUN bit*/
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* update page index to last page */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = (PG_PER_SLC_BLK - 1);

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = (PG_PER_SLC_BLK - 1);
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_REBUILD_CHECK_LAST_PAGE;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_LAST_PAGE:
    {
        /* wait for flash idle */
        if (FALSE== L2_TB_Rebuild_WaitAllFlashFinish(ucSuperPuNum, pStatus))
        {
            return ulRet;
        }

        if (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_Rebuild_TableCheckLastPage(ucSuperPuNum);	
        }
        else if (pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_Rebuild_TableLastPageEmpty(ucSuperPuNum);
        }
        else if (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_Rebuild_TableLastPageAllUECC(ucSuperPuNum);
        }
        else if ((pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] == 0) && (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] != 0)
            && (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] != 0))
        {
            /* some success + some UECC + no empty */
            *pStatus = L2_TB_Rebuild_TableLastPageNoEmpty(ucSuperPuNum);
        }
        else if (pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] != 0)
        {
            /* last page dummy write case: success or UECC + any Lun empty
                       1. read Lun flash status = success + UECC + empty
                       2. read Lun flash status = success + empty
                       3. read Lun flash status = UECC + empty 
                       AT1 not need to dummy write*/
            DBG_Printf("table Blk last pg need dummy write\n");
            *pStatus = L2_TB_Rebuild_TablePrepareDummyWriteInfor(ucSuperPuNum);
        }
        else
        {
            DBG_Printf("[%s]SPU %d invalid case, SucCnt %d, EmptyCnt %d, UECCCnt %d\n", __FUNCTION__,
                ucSuperPuNum, pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum], 
                pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum], 
                pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum]);
            DBG_Getch();
        }
        /*reset Lun load bitmap and flash read infor variable*/
        L2_TB_Rebuild_TableResetParam(ucSuperPuNum); 
    }
        break;

    case L2_TB_REBUILD_READ_NEXT_PAGE:
    {
        /* read next page redundant of each block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            /* read next page redundant of each block */
            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                /*current LUN can send command, mark bitmap corresponding LUN bit*/
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* update page index to next page */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]++;

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);
            }
      	}

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_REBUILD_CHECK_NEXT_PAGE;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_NEXT_PAGE:
    {
        /* wait for flash idle */
        if (FALSE== L2_TB_Rebuild_WaitAllFlashFinish(ucSuperPuNum, pStatus))
        {
            return ulRet;
        }

        /* check flash read information*/
        if (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            /* all LUN read success except RT block*/
            *pStatus = L2_TB_Rebuild_TableCheckNextPage(ucSuperPuNum);	
        }
        else if (pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            *pStatus =  L2_TB_Rebuild_TableNextPageEmpty(ucSuperPuNum);
        }
        else if (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_Rebuild_TableNextPageAllUECC(ucSuperPuNum);
        }
        else if ((pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] == 0) && (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] != 0)
            && (pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] != 0))
        {
            /* some success + some UECC + no empty*/
            *pStatus = L2_TB_Rebuild_TableNextPageNoEmpty(ucSuperPuNum);
        }
        else if (pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] != 0)
        {
            /* next page dummy write case: success or UECC + any Lun empty
                       1. read Lun flash status = success + UECC + empty
                       2. read Lun flash status = success + empty
                       3. read Lun flash status = UECC + empty */
            DBG_Printf("table Blk next pg need dummy write\n");
            *pStatus = L2_TB_Rebuild_TablePrepareDummyWriteInfor(ucSuperPuNum);
            /* current page need dummy write means that it is PPO before abnormal shutdown, set next page is PPO */
            //L2_TB_Rebuild_TableSetNextPageTargetInfor(ucSuperPuNum);
        }
        else
        {
            DBG_Printf("[%s]SPU %d invalid case, SucCnt %d, EmptyCnt %d, UECCCnt %d\n", __FUNCTION__,
                ucSuperPuNum, pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum], 
                pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum], 
                pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum]);
            DBG_Getch();
        }
        L2_TB_Rebuild_TableResetParam(ucSuperPuNum); 
    }
        break;

    case L2_TB_REBUILD_DUMMY_WRITE:
    {
        L2_TB_Rebuild_FillDummyWriteInfor(ucSuperPuNum, &DummyWriteSpare);

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBRebManagement->ulTempLunBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_LPNInPage = 0;

                DBG_Printf("send dummy wr:Lun%d table blk %d pg %d\n", ucLunInSuperPu, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);
                L2_FtlWriteLocal(&FlashAddr, (U32 *)g_L2TempBufferAddr, (U32*)&DummyWriteSpare, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), FALSE, TRUE,NULL);
                dummyWriteCnt[ucSuperPuNum]++;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum]))
        {
            pTBRebManagement->ulLunDummyWrBitmap[ucSuperPuNum] = ~(pTBRebManagement->ulTempLunBitmap[ucSuperPuNum]);
            *pStatus = L2_TB_REBUILD_CHECK_DUMMY_WRITE;
        }
    }
        break;
    case L2_TB_REBUILD_CHECK_DUMMY_WRITE:
    {
        if (FALSE == L2_TB_Rebuild_WaitDummyWrite(ucSuperPuNum))
        {
            return ulRet;
        }

        if (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] == dummyWriteCnt[ucSuperPuNum])
        {
            if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < PG_PER_SLC_BLK - 1)
            {
                DBG_Printf("Table blk %d dummy wr pg%d done, go read next page\n", pTBRebManagement->usBlockIndex[ucSuperPuNum][0],
                    pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
                *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
            }
            else
            {
                DBG_Printf("Table blk %d dummy wr last pg%d done, finish curblk\n", pTBRebManagement->usBlockIndex[ucSuperPuNum][0],
                    pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
                *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
            }
        }
        else
        {
            /*Some Lun program error, not need to move block because this page is dummy, read UECC is no effect */
            if (pTBRebManagement->usPageIndex[ucSuperPuNum][0] < PG_PER_SLC_BLK - 1)
            {
                DBG_Printf("Table blk %d some Lun dummy wr pg%d fail, read next page\n", pTBRebManagement->usBlockIndex[ucSuperPuNum][0],
                    pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
                *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
            }
            else
            {
                DBG_Printf("Table blk %d some lun dummy wr last pg%d fail, finish curblk\n", pTBRebManagement->usBlockIndex[ucSuperPuNum][0],
                    pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
                *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
            }
        }

        dummyWriteCnt[ucSuperPuNum] = 0;
        L2_TB_Rebuild_TableResetParam(ucSuperPuNum);
    }
        break;
    case L2_TB_REBUILD_CURRENT_BLOCK_FINISH: // need double check
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]++;
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
        }

        /* use Lun 0 block index to check the next stage */
        if (pTBRebManagement->usBlockIndex[ucSuperPuNum][0] > g_ulTableBlockEnd[ucSuperPuNum][0])
        {
            *pStatus = L2_TB_REBUILD_READ_ERASEINFO;
            //*pStatus = L2_TB_REBUILD_READ_LATEST_PBIT;
        }
        else
        {
            pTBRebManagement->usUECCErrCnt[ucSuperPuNum][0] = 0;
            *pStatus = L2_TB_REBUILD_READ_FIRST_PAGE;
        }

        L2_TB_Rebuild_TableUpdateBlockTS(ucSuperPuNum);
        L2_TB_Rebuild_TableResetParam(ucSuperPuNum);
    }
        break;

    case L2_TB_REBUILD_READ_LATEST_PBIT:
    {
        /* read latest PBIT for rool back erase count */
        /* only need to check LUN0, if LUN0 doesn't have the PBIT address information, goto read rsved block */
        /* if LUN0 have PBIT information, send read command to all LUNs */
        if ((INVALID_4F != pRT->m_RT[ucSuperPuNum].PBITBlock[0][0]) && ((INVALID_4F != pRT->m_RT[ucSuperPuNum].PBITPPO[0])))
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
                {
                    continue;
                }

                if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                {
                    /* get flashaddr, temp red and send read request to L3, Lun 0 page index is represented to PBIT page index */
                    L2_TB_LoadGetFlashAddr(PAGE_TYPE_PBIT, ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usPageIndex[ucSuperPuNum][0], &FlashAddr);
                    pRed = L2_TB_GetRedAddr(PAGE_TYPE_PBIT, ucSuperPuNum, ucLunInSuperPu);
                    ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);

                    L2_FtlReadLocal((U32 *)ulBufferAddr, &FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, LPN_PER_BUF, 0, TRUE, TRUE);
                    L2_PrintTBFullRecovery((U16)(*pStatus), ucSuperPuNum, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], pTBRebManagement->usPageIndex[ucSuperPuNum][0]);
                    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                }
            }

            if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
            {
                *pStatus = L2_TB_REBUILD_WAIT_LATEST_PBIT;
            }
        }
        else
        {
            /* do not found PBIT, just go to next step */
            *pStatus = L2_TB_REBUILD_READ_RSVD_BLOCK;
        }
    }
        break;

    case L2_TB_REBUILD_WAIT_LATEST_PBIT:
    {
        /* wait for flash idle */
        if (FALSE== L2_TB_Rebuild_WaitAllFlashFinish(ucSuperPuNum, pStatus))
        {
            return ulRet;
        }

        if (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            /* roll back Erase Count */
            if (TRUE == L2_TB_Rebuild_TableRollBackEraseCnt(ucSuperPuNum))
            {
                *pStatus = L2_TB_REBUILD_READ_RSVD_BLOCK;
            }
            else
            {
                *pStatus = L2_TB_REBUILD_READ_LATEST_PBIT;
            }
        }
        else
        {
            /* rebuild PBIT page UECC or EMPTY_PG - may be possible,load RED success but load whole page UECC */
            DBG_Printf("[%s] Load latest PBIT UECC or EMPTY_PG,SPU%d Blk %d PPO %d\n", __FUNCTION__, ucSuperPuNum, 
                pRT->m_RT[ucSuperPuNum].PBITBlock[0][0], pRT->m_RT[ucSuperPuNum].PBITPPO[0]);
            *pStatus = L2_TB_REBUILD_READ_RSVD_BLOCK;
        }

        /*reset Lun load bitmap and flash read infor variable*/
        L2_TB_Rebuild_TableResetParam(ucSuperPuNum);
    }
        break;

    case L2_TB_REBUILD_READ_RSVD_BLOCK:
    {
        /* read 1st page redundant of reserved block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

            L2_PrintTBFullRecovery((U16)(*pStatus), ucSuperPuNum, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);

            if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                ;
            }
            else if (TRUE == pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError)
            {
                /* go on checking next block */
                pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]++;
            }
            else if (TRUE == pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bReserved)
            {
                /*current LUN can send command, mark bitmap corresponding LUN bit*/
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
				 
                /* find reserved block then read */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                L2_TableLoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);
            }
            else
            {
                DBG_Printf("[%s]SPU%d Lun%d READ_RSVD_BLOCK%d ERROR!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, usBlock);
                DBG_Getch();
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_REBUILD_WAIT_RSVD_BLOCK;
        }
    }
        break;

    case L2_TB_REBUILD_WAIT_RSVD_BLOCK:
    {
        /* wait for flash idle */
        if (FALSE== L2_TB_Rebuild_WaitAllFlashFinish(ucSuperPuNum, pStatus))
        {
            return ulRet;
        }

        if (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_Set_TableBlock_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
                L2_TB_Rebuild_TableCheckRsvdBlock(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
            *pStatus = L2_TB_REBUILD_SUCCESS;
        }
        else
        {
            /* reserved block may be two type: 1. empty block 2. trace block data,
                   both of them, we don't do anything */
            *pStatus = L2_TB_REBUILD_SUCCESS;
        }

        /*reset Lun load bitmap and flash read infor variable*/
        L2_TB_Rebuild_TableResetParam(ucSuperPuNum);
    }
        break;

    case L2_TB_REBUILD_READ_ERASEINFO:
    {
        /* Because we need to read last Lun to find the erase infor, use last Lun current AT1 block to check */
        ucLunInSuperPu = LUN_NUM_PER_SUPERPU - 1;
        if (pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] == INVALID_4F)
        {
            usBlock = L2_TB_Rebuild_GetPrevTargetAT1Blk(ucSuperPuNum, ucLunInSuperPu);
            usPPO = PG_PER_SLC_BLK - 1;
        }
        else
        {
            usBlock = pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu];
            usPPO = pRT->m_RT[ucSuperPuNum].CurAT1PPO - 1;
        }

        if (AT1PartialWritePage[ucSuperPuNum] == usPPO)
        {
            DBG_Printf("AT1 PPO - 1 (%d) is Partial write, not need handle erase information\n", usPPO);
            *pStatus = L2_TB_REBUILD_READ_LATEST_PBIT;
			break;
        }

        if ((usBlock < pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][0])
            || (usBlock > pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][AT1_BLOCK_COUNT - 1]))
        {
            DBG_Printf("Read AT1 latest PMT Lun%d blk %d error pg %d\n", ucLunInSuperPu, usBlock, usPPO);
            DBG_Getch();
        }

		if (usPPO >= PG_PER_SLC_BLK)
        {
            DBG_Printf("Read AT1 latest PMT Lun%d blk %d pg %d error\n", ucLunInSuperPu, usBlock, usPPO);
            DBG_Getch();
        }

        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, LUN_NUM_PER_SUPERPU - 1)))
        {
            FlashAddr.m_PUSer = ucSuperPuNum;
            FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
            FlashAddr.m_BlockInPU = usBlock;
            FlashAddr.m_PageInBlock = usPPO;
            FlashAddr.m_LPNInPage = 0;

            L2_FtlReadLocal((U32 *)(g_L2TempBufferAddr + ucSuperPuNum * BUF_SIZE), &FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), NULL, LPN_PER_BUF, 0, TRUE, TRUE);
            DBG_Printf("Read latest PMT Lun%d blk %d pg %d\n", ucLunInSuperPu, usBlock, usPPO);
            *pStatus = L2_TB_REBUILD_CHECK_READ_ERASEINFO;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_READ_ERASEINFO:
    {
		ucLunInSuperPu = LUN_NUM_PER_SUPERPU - 1;
        if(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_PENDING)
        {
            return ulRet;
        }
        else if ((pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_RECC) ||
            (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_RETRY_SUCCESS) ||
            (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_SUCCESS))
        {
            U32 EraseStructSize;
            U8 *TempAddr;
            /* check flash read information*/

            EraseStructSize = sizeof(RecordEraseInfo);
            TempAddr = (U8*)(g_L2TempBufferAddr + ucSuperPuNum * BUF_SIZE) + BUF_SIZE - EraseStructSize;
            COM_MemByteCopy((U8 *)&g_NeedEraseBlkInfo[ucSuperPuNum], TempAddr, EraseStructSize);

#ifdef SPOR_DEBUGLOG_ENABLE
            U8 i, j;
            DBG_Printf("EIf:SLC cnt %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt);
            if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt != 0)
            {    
                for (i = 0; i < g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt; i++)
                {
                    DBG_Printf("SLC VB%d\n", g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCVBN[i]);
                    for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
                        DBG_Printf("Lun %d PB %d\n", j, g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCPBN[j][i]);
                }
            }

            DBG_Printf("EIF:TLC cnt %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt);
            if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt != 0)
            {
                for (i = 0; i < g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt; i++)
                {
                    DBG_Printf("TLC VB%d\n", g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCVBN[i]);
                    for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
                        DBG_Printf("Lun %d PB %d\n", j, g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCPBN[j][i]);
                }
            }
#endif
            if ((g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt + g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt) != 0)
            {
                DBG_Printf("need extra handling\n");
                bHandleSPORDuringErase[ucSuperPuNum] = TRUE;
            }
        }
        else if (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_EMPTY_PG)
        {
            DBG_Printf("AT1 PPO-1 empty\n");
			bHandleSPORDuringErase[ucSuperPuNum] = FALSE;
        }
        else if (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_FAIL)
        {
            DBG_Printf("AT1 PPO-1 UECC\n");
            bHandleSPORDuringErase[ucSuperPuNum] = FALSE;
        }
        else
        {
            DBG_Printf("AT1 PPO-1 no match status %d\n", pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]);
			DBG_Getch();
        }

		L2_TB_Rebuild_TableResetParam(ucSuperPuNum);

        *pStatus = L2_TB_REBUILD_READ_LATEST_PBIT;
    }
        break;

    case L2_TB_REBUILD_ERROR_ERASE_ONE_BLOCK:
    {
        if (L2_TB_SUCCESS == L2_TB_Rebuild_TableBlockErase(ucSuperPuNum))
        {
            *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
        break;
#if 0
    case L2_TB_REBUILD_DUMMY_WRITE_FAIL_MOVEBLOCK:
    {
        ucMoveSuccess = 0;
        /* program fail - errorhandling move block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->usReadSuccessLun[ucSuperPuNum][ucLunInSuperPu] == TRUE)
            {
                ucMoveSuccess++;
                 continue;
            }

            ulCmdRet = L2_TableErrorHandlingMoveBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 0, WRITE_ERR);
            if (L2_ERR_HANDLING_PENDING == ulCmdRet)
            {
                ;
            }
            else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
            {
                if (bIsUpdate[ucLunInSuperPu] != TRUE)
                {
                    L2_RT_Rebuild_MarkTable(ucSuperPuNum, ucLunInSuperPu);
                    bIsUpdate[ucLunInSuperPu] = TRUE;
                }
                ucMoveSuccess++;
            }
            else
            {
                DBG_Printf("[%s] SuperPU%d Lun%d write ErrorHandling Fail\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                DBG_Getch();
            }
        }

        if (ucMoveSuccess == LUN_NUM_PER_SUPERPU)
        {
            *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
            L2_TB_Rebuild_TableResetParam(ucSuperPuNum);
        }
    }
        break;
#endif
    case L2_TB_REBUILD_ERROR_ERASE_ALL:
    {
        ucEraseSucCnt = 0;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (L2_TB_SUCCESS == L2_TB_Rebuild_TableBlockEraseAll(ucSuperPuNum, ucLunInSuperPu))
            {
                ucEraseSucCnt++;
            }
        }

        if (ucEraseSucCnt == LUN_NUM_PER_SUPERPU)
        {
            /* init parameter*/
            L2_TB_Rebuild_MarkBadBlock(ucSuperPuNum);
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                L2_RT_Rebuild_MarkTable(ucSuperPuNum, ucLunInSuperPu);
            }

            pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp = 0;
            bTBRebuildEraseAll[ucSuperPuNum] = FALSE;
            *pStatus = L2_TB_REBUILD_SUCCESS;
        }

    }
        break;

    case L2_TB_REBUILD_FAIL:
    {
        if (TRUE == bTBRebuildEraseAll[ucSuperPuNum])
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pTBRebManagement->EraseAllStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERASEALL_START;
            }

            *pStatus = L2_TB_REBUILD_ERROR_ERASE_ALL;
        }
        else
        {
            ulRet = L2_TB_FAIL;
        }
    }
        break;

    case L2_TB_REBUILD_SUCCESS:
    {
        if (TRUE == bTBRebuildEraseAll[ucSuperPuNum])
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                pTBRebManagement->EraseAllStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERASEALL_START;
            }

            *pStatus = L2_TB_REBUILD_ERROR_ERASE_ALL;
        }
        else
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                L2_TB_Rebuild_TableCheckRT(ucSuperPuNum, ucLunInSuperPu);
                if (INVALID_4F == pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu])
                {
                    DBG_Printf("[%s]FAIL CurAT0Block is INVALID_4F, SPU:%d, Lun:%d\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                    bTBRebuildEraseAll[ucSuperPuNum] = TRUE;
                }
            }

            if (bTBRebuildEraseAll[ucSuperPuNum] == TRUE)
            {
                *pStatus = L2_TB_REBUILD_ERROR_ERASE_ALL;
            }
            else
            {
                ulRet = L2_TB_SUCCESS;
            }

        }
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_Rebuild_TableBlock invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_CheckAT1TargetBlk(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucIndex;

    if(pRT->m_RT[ucSuperPuNum].CurAT1PPO != INVALID_4F)
    {
        /* there has open AT1 block, retrun directly */
        return TRUE;
    }

    if (FirstPageEmptyAT1Index[ucSuperPuNum] == INVALID_2F)
    {
        /* special case:all AT1 block is closed */
        DBG_Printf("[%s]All AT1 block is closed, not need dummy write\n", __FUNCTION__);
        return FALSE;
    }
    else
    {
        ucIndex = FirstPageEmptyAT1Index[ucSuperPuNum];
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {   
            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] =  pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][ucIndex];
        }

        pRT->m_RT[ucSuperPuNum].CurAT1PPO = 0; 

        return TRUE;
    }
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_HandleEraseInfo(U8 ucSuperPuNum)
{
    BOOL bNeedErase = FALSE;
    U8 ucLunInSuperPu, ucTargetType;
    U16 usPhyBlk, usCurTargetVBN, usCurTargetPBN;
    BOOL bIsTarget = FALSE;

    if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt + g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt == 0)
    {
        DBG_Printf("SPU %d has no Erase blk need to handle error\n", ucSuperPuNum);
        DBG_Getch();
    }
    DBG_Printf("SLCIndex %d, SLC erase cnt %d, TLC erase cnt %d\n", SLCIndex[ucSuperPuNum], g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt, g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt);
    while (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt != 0)
    {
        usPhyBlk = g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCPBN[0][SLCIndex[ucSuperPuNum]];
		/* use target block Lun 0 PBN to check, because after resume,VBN-PBN mapping will be changed */
        for (ucTargetType = 0; ucTargetType < (TARGET_ALL - 1); ucTargetType++)
        {
            usCurTargetVBN = pPBIT[ucSuperPuNum]->m_TargetBlock[ucTargetType];
            usCurTargetPBN = pVBT[ucSuperPuNum]->m_VBT[usCurTargetVBN].PhysicalBlockAddr[0];
            if (usPhyBlk == usCurTargetPBN)
            {
                bIsTarget = TRUE;
                SLCIndex[ucSuperPuNum]++;
                g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt--;
#ifdef SPOR_DEBUGLOG_ENABLE
                DBG_Printf("PBN %d is target, SLCIndex %d\n", usPhyBlk, SLCIndex[ucSuperPuNum]);
#endif
                break;
            }
        }

        if (bIsTarget != TRUE)
        {
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("[%s]need erase SLC VBN %d, index %d\n", __FUNCTION__, g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCVBN[SLCIndex[ucSuperPuNum]], SLCIndex[ucSuperPuNum]);
#endif
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usPhyBlk = g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCPBN[ucLunInSuperPu][SLCIndex[ucSuperPuNum]];
                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usPhyBlk;
#ifdef SPOR_DEBUGLOG_ENABLE
                DBG_Printf("Lun %d PB %d\n", ucLunInSuperPu, usPhyBlk);
#endif
				if (usPhyBlk == INVALID_4F)
					DBG_Getch();
            }
            
            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu - 1][usPhyBlk].bFree == FALSE)
            {
                DBG_Printf("L%d PB %d erase candidate blk not free, adjust SLC alloCnt %d(old)_%d(new)\n", ucLunInSuperPu - 1, usPhyBlk, pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_SLC], (pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_SLC] - 1));
                pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_SLC]--;
#ifdef SIM
                L3_DebugPBIT_Decrease_AllocateBlkCnt(ucSuperPuNum, BLKTYPE_SLC);
                L3_DebugPuInfo_Increase_SLCFreePageCnt(ucSuperPuNum);
#endif
            }

            bNeedErase = TRUE;
            SLCIndex[ucSuperPuNum]++;
            g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt--;
            return bNeedErase;
        }
        else
        {
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("SLC VBN %d index %d is target, cannot be erased\n", g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCVBN[SLCIndex[ucSuperPuNum]], SLCIndex[ucSuperPuNum]);
#endif
            if (SLCIndex[ucSuperPuNum] == MAX_ERASE_SLCBLK_CNT)
                break;
            else
                bIsTarget = FALSE;
        }
    }
	
    if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt != 0)
    {
#ifdef SPOR_DEBUGLOG_ENABLE
        DBG_Printf("[%s]need erase TLC VBN %d\n", __FUNCTION__, g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCVBN[0]);
#endif
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            usPhyBlk = g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCPBN[ucLunInSuperPu][0];
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usPhyBlk;
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("Lun %d PB %d\n", ucLunInSuperPu, usPhyBlk);
#endif
			if (usPhyBlk == INVALID_4F)
				DBG_Getch();
        }

        if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu - 1][usPhyBlk].bFree == FALSE)
        {
			DBG_Printf("L%d PB %d erase candidate blk not free, adjust TLC alloCnt %d(old)_%d(new)\n", ucLunInSuperPu - 1, usPhyBlk, pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_TLC], (pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_TLC] - 1));
            pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_TLC]--;
#ifdef SIM
            L3_DebugPBIT_Decrease_AllocateBlkCnt(ucSuperPuNum, BLKTYPE_TLC);
#endif
        }

        bNeedErase = TRUE;
        g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt--;
    }

    return bNeedErase;
}

L2_TB_REBUILD_CHECK_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_IsNeedSaveBBT(U8 ucSuperPuNum)
{
    if (bSaveBBT[ucSuperPuNum] == TRUE)
    {
        return L2_TB_REBUILD_CHECK_ERASE_SAVE_BBT;
    }
    else
   	{
   	    if (bHandleSPORDuringErase[ucSuperPuNum] == FALSE)
            return L2_TB_REBUILD_CHECK_SUCCESS;
        else
        {
            HAL_DMAESetValue(g_L2TempBufferAddr, BUF_SIZE, 0);
            return L2_TB_REBUILD_CHECK_DUMMYWRITE_AT1;
        }
   	}
}

/* rebuild data block start */
void MCU1_DRAM_TEXT L2_TB_Rebuild_DataBlockSetFree(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usBlock)
{
    L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
    L2_PBIT_Set_Allocate(ucSuperPuNum, ucLunInSuperPu, usBlock, FALSE);

    pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usBlock].LastPageTimeStamp = 0; // need double check: it can be delete

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataBlockRollBackTLCSetAllocated(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPhyBlock, U16 usVirBlock)
{
    L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, INVALID_4F);
    L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlock, INVALID_4F);
    //pVBT[ucSuperPuNum]->m_VBT[usVirBlock].bPhyAddFindInFullRecovery = FALSE;
    pVBT[ucSuperPuNum]->m_VBT[usVirBlock].bPhyAddFindInFullRecovery &= ~(0x1<<ucLunInSuperPu);

    L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, TRUE);
    L2_PBIT_Set_Allocate(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, FALSE);

    L2_VBT_Set_TLC(ucSuperPuNum, usVirBlock, FALSE);
    L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, FALSE);
    pVBT[ucSuperPuNum]->m_VBT[usVirBlock].VBTType = VBT_TYPE_INVALID;

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataBlockSetAllocated(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPhyBlock, U16 usVirBlock, BOOL bSLCBlock, BLOCK_TYPE BlockType)
{
    L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, usVirBlock);
    L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlock, usPhyBlock);
    pVBT[ucSuperPuNum]->m_VBT[usVirBlock].bPhyAddFindInFullRecovery |= (0x1<<ucLunInSuperPu);

    L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, FALSE);
    L2_PBIT_Set_Allocate(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, TRUE);

    if (TRUE == bSLCBlock)
    {
        //L2_PBIT_Increase_AllocBlockCnt(ucSuperPuNum, BLKTYPE_SLC);//nick delete
    }
    else
    {
        //set TLC flag in VBT & PBIT
        L2_VBT_Set_TLC(ucSuperPuNum, usVirBlock, TRUE);// not set here?
        L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, usPhyBlock, TRUE);


        //FIRMWARE_LogInfo("Allocated Blk : TLC Pu %d Allocate Blk %d ,Now allocated tlc sum %d\n ",ucPuNum,usVirBlock,
        //pPBIT[ucPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_TLC]);
    }

    //FIRMWARE_LogInfo("L2_TB_Rebuild_DataBlockSetAllocated Pu %d bSLCBlock %d VirBlk %d PhyBlk %d TotalBlkCnt %d SLCBlkCnt %d TLCBlkCnt %d\n", ucPuNum,
    //    bSLCBlock, usVirBlock, usPhyBlock, pPBIT[ucPuNum]->m_TotalDataBlockCnt[BLKTYPE_SLC], pPBIT[ucPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_SLC], pPBIT[ucPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_TLC]);

    switch (BlockType)
    {
    case BLOCK_TYPE_SEQ:
    case BLOCK_TYPE_RAN:
        pVBT[ucSuperPuNum]->m_VBT[usVirBlock].VBTType = VBT_TYPE_HOST;
        break;

    case BLOCK_TYPE_TLC_GC:
    //case BLOCK_TYPE_TLC_SWL:
    case BLOCK_TYPE_TLC_W:
        pVBT[ucSuperPuNum]->m_VBT[usVirBlock].VBTType = VBT_TYPE_TLCW;
        break;
    }
    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataBlockSwap(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usVirBlock, U16 usOriPhyBlock, U16 usNewPhyBlock)
{
    L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usOriPhyBlock, INVALID_4F);
    L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlock, usNewPhyBlock);
    L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usNewPhyBlock, usVirBlock);

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_AllocatedFreeSetTable(U8 ucSuperPuNum, U16 VirtualBlockAddr, BOOL bHaveFreePhyBlkFlag, BOOL bIsTLCBlk)
{
    U8 ucLunInSuperPu;

    /* all Luns have Phyblk can be allocated, set PBIT and VBT */
    if (bHaveFreePhyBlkFlag == TRUE)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, g_ulTempLunPhyBlk[ucLunInSuperPu], VirtualBlockAddr);
            L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, g_ulTempLunPhyBlk[ucLunInSuperPu]);
            pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bPhyAddFindInFullRecovery |= (0x1 << ucLunInSuperPu);

            if (bIsTLCBlk == TRUE)
            {
                L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, g_ulTempLunPhyBlk[ucLunInSuperPu], TRUE);
            }
            else
            {
                L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, g_ulTempLunPhyBlk[ucLunInSuperPu], FALSE);
            }
        }
  
        pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bFree = TRUE;
        if (bIsTLCBlk == TRUE)
        {
            L2_VBT_Set_TLC(ucSuperPuNum, VirtualBlockAddr, TRUE);
            L2_PBIT_Increase_TotalBlockCnt(ucSuperPuNum, BLKTYPE_TLC);
        }
        else
        {
            L2_VBT_Set_TLC(ucSuperPuNum, VirtualBlockAddr, FALSE);
            L2_PBIT_Increase_TotalBlockCnt(ucSuperPuNum, BLKTYPE_SLC);
        }
    }
    else
    {
        /* any Lun no free phyblk can be allocated, reset current virtual block setting */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, INVALID_4F);
        }
        pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bPhyAddFindInFullRecovery = 0;
    }
}

U32 MCU1_DRAM_TEXT L2_TB_Rebuild_AllocatedFreeRsvdBlk(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U32 ulPhyBlk, ulStartBlk, ulTotalRsvedBlkCnt;
    PBIT_ENTRY* pPBITItem = pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu];

    ulStartBlk = g_PuInfo[ucSuperPuNum]->m_SLCEndPhyBlk[ucLunInSuperPu] + 1;
    ulTotalRsvedBlkCnt = g_PuInfo[ucSuperPuNum]->m_TLCStartPhyBlk[ucLunInSuperPu] - g_PuInfo[ucSuperPuNum]->m_SLCEndPhyBlk[ucLunInSuperPu] - 1;

    for (ulPhyBlk = ulStartBlk; ulPhyBlk < ulTotalRsvedBlkCnt; ulPhyBlk ++)
    {
        if (pPBITItem[ulPhyBlk].bBackup == FALSE)
            continue;

        if (pPBITItem[ulPhyBlk].bError == TRUE)
            continue;

        if (pPBITItem[ulPhyBlk].bFree == FALSE)
            continue;

		break;
    }

    if (ulPhyBlk == g_PuInfo[ucSuperPuNum]->m_TLCStartPhyBlk[ucLunInSuperPu])
        return INVALID_4F;
    else
        return ulPhyBlk;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataAllocateFreeBlock(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;
    U32 PhyBlockAddr, VirtualBlockAddr;
    U32 uFirstPhyBlkOfData, uTotalTLCBlkCnt = 0, uTotalSLCBlkCnt = 0;
    BOOL bHaveFreePhyBlkFlag = TRUE;

    uFirstPhyBlkOfData = TABLE_BLOCK_COUNT;
    /***************allocate TLC Blk*************************/
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu] = BLK_PER_PLN + RSV_BLK_PER_PLN - 1;  //search physical blk begin
    }

    for (VirtualBlockAddr = 0; VirtualBlockAddr < TLC_BLK_CNT; VirtualBlockAddr++)
    {
        /*check virtual block LUN bitmap, not 0: has mapping; 0: no mapping and need allocate free block*/
        if (pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bPhyAddFindInFullRecovery != FALSE)
        {
            uTotalTLCBlkCnt ++;
            continue;
        }

        if (TRUE == bHaveFreePhyBlkFlag) //have physical blk
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                g_ulTempLunPhyBlk[ucLunInSuperPu] = INVALID_4F;
                //find a good physical blk
                for (PhyBlockAddr = g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu]; PhyBlockAddr >= g_PuInfo[ucSuperPuNum]->m_TLCStartPhyBlk[ucLunInSuperPu]; PhyBlockAddr--)
                {
                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == FALSE)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bError == TRUE)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bAllocated == TRUE)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].VirtualBlockAddr != INVALID_4F)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == TRUE)
                    {
                        g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu] = PhyBlockAddr-1; //next seach physical blk begin
                        g_ulTempLunPhyBlk[ucLunInSuperPu] = PhyBlockAddr;
                        break;
                    }
                }

                /*one LUN cannot find the free physical block, try to fine the free reserved block to allocate. If no reserved block can be used,
                             set current VBN and PBN mapping to invalid. */
                if (g_ulTempLunPhyBlk[ucLunInSuperPu] == INVALID_4F)
                {
                    DBG_Printf("[%s]SPU%d VB %d LUN %d cannot find free TLC PB, try to allocate rsvdblk\n", __FUNCTION__, ucSuperPuNum, VirtualBlockAddr, ucLunInSuperPu);
                    g_ulTempLunPhyBlk[ucLunInSuperPu] = L2_TB_Rebuild_AllocatedFreeRsvdBlk(ucSuperPuNum, ucLunInSuperPu);
                    if (g_ulTempLunPhyBlk[ucLunInSuperPu] == INVALID_4F)
                    {
                        DBG_Printf("no free rsvdblk\n");
                        bHaveFreePhyBlkFlag = FALSE;
                        L2_TB_Rebuild_AllocatedFreeSetTable(ucSuperPuNum, VirtualBlockAddr, bHaveFreePhyBlkFlag, TRUE);
                        break;
                    }
                    DBG_Printf("find free rsvdblk %d\n", g_ulTempLunPhyBlk[ucLunInSuperPu]);
                }
            }

            if (bHaveFreePhyBlkFlag == TRUE)
            {
                uTotalTLCBlkCnt++;
                L2_TB_Rebuild_AllocatedFreeSetTable(ucSuperPuNum, VirtualBlockAddr, bHaveFreePhyBlkFlag, TRUE);
            }
        }
        else  //no free and reserved physical TLC blk
        {
            L2_TB_Rebuild_AllocatedFreeSetTable(ucSuperPuNum, VirtualBlockAddr, bHaveFreePhyBlkFlag, TRUE);
        }
    }

    /* reset each Lun temp phy block to SLC start physical block number */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu] = uFirstPhyBlkOfData;  //search physical blk begin
    }

    /*************************allocat SLC blk*****************************/
    bHaveFreePhyBlkFlag = TRUE;
    for (VirtualBlockAddr = TLC_BLK_CNT; VirtualBlockAddr < VIR_BLK_CNT; VirtualBlockAddr++)
    {
        //if have rebuild PBIT ,no need to handle
        if (pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bPhyAddFindInFullRecovery != FALSE)
        {
            uTotalSLCBlkCnt++;
            continue;
        }

        if (TRUE == bHaveFreePhyBlkFlag) //have physical blk
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                g_ulTempLunPhyBlk[ucLunInSuperPu] = INVALID_4F;
                //find a good physical blk
                for (PhyBlockAddr =  g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu]; PhyBlockAddr <= g_PuInfo[ucSuperPuNum]->m_SLCEndPhyBlk[ucLunInSuperPu]; PhyBlockAddr ++)
                {
                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == FALSE)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bError == TRUE)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bAllocated == TRUE)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].VirtualBlockAddr != INVALID_4F)
                        continue;

                    if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == TRUE)
                    {
                        g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu] = PhyBlockAddr; //next seach physical blk begin
                        g_ulTempLunPhyBlk[ucLunInSuperPu] = PhyBlockAddr;
                        break;
                    }
                }

                /*one LUN cannot find the free physical block, try to fine the free reserved block to allocate. If no reserved block can be used,
                             set current VBN and PBN mapping to invalid. */
                if (g_ulTempLunPhyBlk[ucLunInSuperPu] == INVALID_4F)
                {
                    DBG_Printf("[%s]SPU%d VB%d LUN %d cannot find the free SLC phyblk, try to allocate rsvdblk\n", __FUNCTION__, ucSuperPuNum, VirtualBlockAddr, ucLunInSuperPu, PhyBlockAddr);
                    g_ulTempLunPhyBlk[ucLunInSuperPu] = L2_TB_Rebuild_AllocatedFreeRsvdBlk(ucSuperPuNum, ucLunInSuperPu);
                    if (g_ulTempLunPhyBlk[ucLunInSuperPu] == INVALID_4F)
                    {
                        DBG_Printf("no free rsvdblk\n");
                        bHaveFreePhyBlkFlag = FALSE;
                        L2_TB_Rebuild_AllocatedFreeSetTable(ucSuperPuNum, VirtualBlockAddr, bHaveFreePhyBlkFlag, FALSE);
                        break;
                    }

                    DBG_Printf("find free rsvdblk %d\n", g_ulTempLunPhyBlk[ucLunInSuperPu]);
                }
            }

            if (bHaveFreePhyBlkFlag == TRUE)
            {
                uTotalSLCBlkCnt++;
                L2_TB_Rebuild_AllocatedFreeSetTable(ucSuperPuNum, VirtualBlockAddr, bHaveFreePhyBlkFlag, FALSE);
            }
        }
        else  //no physical blk
        {
            L2_TB_Rebuild_AllocatedFreeSetTable(ucSuperPuNum, VirtualBlockAddr, bHaveFreePhyBlkFlag, FALSE);
        }
    }

    /************************allocate Backup Blk***************************/
	for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (PhyBlockAddr = (g_PuInfo[ucSuperPuNum]->m_SLCEndPhyBlk[ucLunInSuperPu]+1); PhyBlockAddr < g_PuInfo[ucSuperPuNum]->m_TLCStartPhyBlk[ucLunInSuperPu] ; PhyBlockAddr++)
        {
            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bError == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == FALSE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bAllocated == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].VirtualBlockAddr != INVALID_4F)
                continue;

            L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, TRUE);
            L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, INVALID_4F);
            L2_PBIT_Set_Backup(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, TRUE);
        }
	}

    /************************Set SLC broken Blk***************************/
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (PhyBlockAddr = uFirstPhyBlkOfData; PhyBlockAddr <= g_PuInfo[ucSuperPuNum]->m_SLCEndPhyBlk[ucLunInSuperPu] ; PhyBlockAddr++)
        {
            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bError == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == FALSE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bAllocated == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].VirtualBlockAddr != INVALID_4F)
                continue;

            L2_PBIT_Set_Broken(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, TRUE);
            L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, INVALID_4F);
        }
    }
    
    /************************Set TLC broken Blk***************************/
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu] = BLK_PER_PLN + RSV_BLK_PER_PLN - 1;  //search physical blk begin
    }

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (PhyBlockAddr = g_ulTempPhyBlkStart[ucSuperPuNum][ucLunInSuperPu]; PhyBlockAddr >= g_PuInfo[ucSuperPuNum]->m_TLCStartPhyBlk[ucLunInSuperPu] ; PhyBlockAddr--)
        {
            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bError == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == FALSE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bAllocated == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].VirtualBlockAddr != INVALID_4F)
                continue;

            L2_PBIT_Set_Broken(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, TRUE);
            L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, INVALID_4F);
        }
    }

    /***************************Set Invalid Virtual Blk1362~1377***************************************/
    for (VirtualBlockAddr = VIR_BLK_CNT; VirtualBlockAddr < BLK_PER_PLN; VirtualBlockAddr++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, INVALID_4F);
        }
    }

    //adjust total blk cnt
    pPBIT[ucSuperPuNum]->m_TotalDataBlockCnt[BLKTYPE_SLC] = uTotalSLCBlkCnt;
    pPBIT[ucSuperPuNum]->m_TotalDataBlockCnt[BLKTYPE_TLC] = uTotalTLCBlkCnt;

#ifdef SIM
    if (uTotalSLCBlkCnt != SLC_BLK_CNT)
    {
        DBG_Printf("Pu %d SLCBlkCnt %d SLC_BLK_CNT %d uTotalTLCBlkCnt %d Wrong\n", ucSuperPuNum, uTotalSLCBlkCnt, SLC_BLK_CNT, uTotalTLCBlkCnt);
        //DBG_Getch();
    }
#endif

    FIRMWARE_LogInfo("L2_TB_Rebuild_DataAllocateFreeBlock Pu %d SLCBlkCnt %d uTotalTLCBlkCnt %d\n", ucSuperPuNum, uTotalSLCBlkCnt, uTotalTLCBlkCnt);

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataSetOtherRPMTInfo(U8 ucSuperPuNum, U8 ucLunInSuperPu, TargetType ucTargetType)
{
    U8 ucTargetIndex;
    U16 usVirBlk;
    RPMT *pRPMT;

    ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
    usVirBlk = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
    /* use Lun0 index to get RPMT start address*/
    pRPMT = (RPMT*)L2_TB_GetDataAddr(PAGE_TYPE_RPMT, ucSuperPuNum, 0, (U16)ucTargetType);
    pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU= ucSuperPuNum;
    pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock= usVirBlk;
#ifdef SPOR_DEBUGLOG_ENABLE
    DBG_Printf("[%s]Lun %d RPMT SPU %d VBN %d\n", __FUNCTION__, ucLunInSuperPu, pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU,
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock);
#endif
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataSetInvalidRPMT(U8 ucSuperPuNum, U8 ucLunInSuperPu, TargetType ucTargetType)
{
    U8   ucLPN;
    U16  usPage;
    RPMT *pRPMT;

    /* use Lun0 index to get RPMT start address*/
    pRPMT = (RPMT*)L2_TB_GetDataAddr(PAGE_TYPE_RPMT, ucSuperPuNum, 0, (U16)ucTargetType);
    usPage = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
    for (ucLPN = 0; ucLPN < LPN_PER_BUF; ucLPN++)
    {
        pRPMT->m_RPMT[ucLunInSuperPu].m_RPMTItems[(usPage*LPN_PER_BUF) + ucLPN] = INVALID_8F;
    }

    pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usPage] = INVALID_8F;
    pRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[usPage] = INVALID_8F;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataSetRPMT(U8 ucSuperPuNum, U8 ucLunInSuperPu, RED *pRed)
{
    U8   ucLPN;
    U8   ucTargetType;
    U16  usPage;
    RPMT *pRPMT;

    ucTargetType = pRed->m_RedComm.bcBlockType - BLOCK_TYPE_SEQ;
    if (TEMP_BLK_CNT_PER_TARGET <= pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType])
    {
        //DBG_Printf("[L2_TB_Rebuild_DataSetRPMT]: pu %d, TargetType %d overflowed! \n", ucPuNum, ucTargetType);
        //DBG_Getch();

        /* here must be rebuilding the WL source block */
        return;
    }
    else
    {
        g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType] = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
    }

    /* use Lun0 index to get RPMT start address*/
    pRPMT = (RPMT*)L2_TB_GetDataAddr(PAGE_TYPE_RPMT, ucSuperPuNum, 0, (U16)ucTargetType);
    usPage = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
    for (ucLPN = 0; ucLPN < LPN_PER_BUF; ucLPN++)
    {
        pRPMT->m_RPMT[ucLunInSuperPu].m_RPMTItems[(usPage*LPN_PER_BUF) + ucLPN] = pRed->m_DataRed.aCurrLPN[ucLPN];
    }

    pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usPage] = pRed->m_RedComm.ulTimeStamp;
    pRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[usPage] = pRed->m_RedComm.ulTargetOffsetTS;

    return;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_DataSaveTargetRPMT(U8 ucSuperPuNum, U8 ucLunInSuperPu, U8 ucTargetType, U8 ucTargetIndex)
{
    RPMT *pRPMT;

    g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType] = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
    pRPMT = (RPMT*)L2_TB_GetDataAddr(PAGE_TYPE_RPMT, ucSuperPuNum, 0, (U16)ucTargetType);
    pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock = (U32)pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
    pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU = ucSuperPuNum;
    pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType]++;

    return;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_IsNeedLoadClosedTargetBlkRPMT(U8 ucSuperPuNum, TargetType ucTargetType)
{
    U8  ucLunInSuperPu, ulRPMTBufferPointer, ucTargetIndex;
    BOOL bNeedLoad = FALSE;
    U16 usVirBlk, usPhyBlk;	
    PhysicalAddr Addr;
    RPMT* pRPMTPage;

    if((g_PuInfo[ucSuperPuNum]->m_RPMTFlushBitMap[ucTargetType] != 0) && (bNeedLoadPRMT[ucTargetType] == FALSE))
    {
        bNeedLoad = TRUE;
        //usVirBlk = pPBIT[ucSuperPuNum]->m_TargetBlock[ucTargetType];

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (TRUE == COM_BitMaskGet(pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum], ucLunInSuperPu))
                continue;

            if (FALSE == COM_BitMaskGet(g_PuInfo[ucSuperPuNum]->m_RPMTFlushBitMap[ucTargetType], ucLunInSuperPu))
            {
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
                continue;
            }

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
                if (ucTargetIndex != 0)
                {
                    DBG_Printf("[%s]Lun %d TargetIndex%d is not 0\n", __FUNCTION__,ucLunInSuperPu , ucTargetIndex);
                    DBG_Getch();
                }

                usVirBlk = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
                DBG_Printf("[%s]SPU%d Targettype %d VB %d needs load partial RPMT(bitmap 0x%x)\n", __FUNCTION__, ucSuperPuNum, ucTargetType, usVirBlk,
                    g_PuInfo[ucSuperPuNum]->m_RPMTFlushBitMap[ucTargetType]);

                usPhyBlk = pVBT[ucSuperPuNum]->m_VBT[usVirBlk].PhysicalBlockAddr[ucLunInSuperPu];
                Addr.m_PUSer = ucSuperPuNum;
                Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                Addr.m_BlockInPU = usPhyBlk;
                Addr.m_PageInBlock = (PG_PER_SLC_BLK - 1);
                Addr.m_LPNInPage = 0;

                g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType] = ucTargetIndex;
                ulRPMTBufferPointer = g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType];
                pRPMTPage = g_PuInfo[ucSuperPuNum]->m_pRPMT[ucTargetType][ulRPMTBufferPointer];
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
#ifdef SPOR_DEBUGLOG_ENABLE
                DBG_Printf("Lun %d Blk %d_%d Load RPMT,RPMTBufferPointer %d\n", ucLunInSuperPu, usVirBlk, usPhyBlk, ulRPMTBufferPointer);
#endif
                L2_FtlReadLocal((U32*)&pRPMTPage->m_RPMT[ucLunInSuperPu], &Addr, 
                    &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), NULL, LPN_PER_BUF, 0, FALSE, TRUE);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            bNeedLoadPRMT[ucTargetType] = TRUE;
        }
    }

    return bNeedLoad;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_SortLunOrderTS(U32 *ulLunOrderTS, U32* ucLunOrder)
{
    U8 ucLunInSuperPu, i;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (i = (ucLunInSuperPu + 1); i < LUN_NUM_PER_SUPERPU; i++)
        {
            if (ulLunOrderTS[i] < ulLunOrderTS[ucLunInSuperPu])
            {
                SwapTwoValue((U32 *)&ucLunOrder[i], (U32 *)&ucLunOrder[ucLunInSuperPu]);
                SwapTwoValue((U32 *)&ulLunOrderTS[i], (U32 *)&ulLunOrderTS[ucLunInSuperPu]);
            }
        }
    }
}

U8 MCU1_DRAM_TEXT L2_TB_Rebuild_CheckOrdering(U32 *ulLunOrderTS, U32* ucLunOrder)
{
    U8  i, ucFirstNotSeqIndex;
    U32 ulCurLunOrderTS, ulNextLunOrderTS;

    if (ulLunOrderTS[0] != 0)
    {
        DBG_Printf("[%s] 1st Lun %d LunOrderTS %d is not 0 error\n", __FUNCTION__, ucLunOrder[0], ulLunOrderTS[0]);
        DBG_Getch();
    }

    ucFirstNotSeqIndex = INVALID_2F;
    for (i = 0; i < (LUN_NUM_PER_SUPERPU - 1); i++)
    {
        ulCurLunOrderTS = ulLunOrderTS[i];
        ulNextLunOrderTS = ulLunOrderTS[i+1];

        if (((ulCurLunOrderTS + 1) != ulNextLunOrderTS) && (ulNextLunOrderTS != INVALID_8F))
        {
            ucFirstNotSeqIndex = i + 1;
            break;
        }
        else if ((((ulCurLunOrderTS + 1) != ulNextLunOrderTS) && (ulNextLunOrderTS == INVALID_8F)) || 
			(((ulCurLunOrderTS + 1) == ulNextLunOrderTS)))
        {
            continue;
        }
        else
        {
            DBG_Printf("[%s]i = %d, CurLun %d OrderTS %d and NextLun %d OrderTS %d error case\n", __FUNCTION__, i ,
                ucLunOrder[i], ulCurLunOrderTS, ucLunOrder[i + 1], ulNextLunOrderTS);
            DBG_Getch();
        }
    }

    return ucFirstNotSeqIndex;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_ReAdjustTargetPPO(U8 ucSuperPuNum, U8 ucTargetType, U32 *ulLunOrderTS, U32* ucLunOrder, U8 ucFirstNotSeqIndex)
{
    U8  ucTargetIndex, ucLunInSuperPu, i;
    U16 usTargetPPO;

    for (i = ucFirstNotSeqIndex; i < LUN_NUM_PER_SUPERPU; i++)
    {
        ucLunInSuperPu = ucLunOrder[i];

        if (ulLunOrderTS[i] != INVALID_8F)
        {
            ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
            usTargetPPO = pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
            pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = usTargetPPO - 1;
            g_PuInfo[ucSuperPuNum]->m_pRPMT[ucTargetType][0]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usTargetPPO - 1] = INVALID_8F;
            g_PuInfo[ucSuperPuNum]->m_pRPMT[ucTargetType][0]->m_RPMT[ucLunInSuperPu].m_LunOrderTS[usTargetPPO - 1] = INVALID_8F;
            DBG_Printf("Adjust index %d Lun %d PPO to %d\n", i, ucLunInSuperPu, usTargetPPO - 1);
        }
    }
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_CheckIsTargetPPODataSeq(U8 ucSuperPuNum, U8 ucTargetType)
{
    U8 ucLunInSuperPu, ucTargetIndex, ucFirstNotSeqIndex;
    U16 usCurTargetPPO;
    U32 ulLunOrderTS[LUN_NUM_PER_SUPERPU];
    U32 ucLunOrder[LUN_NUM_PER_SUPERPU];
    RPMT *pTargetRPMT;
    BOOL bTargetPPODataStart;

    bTargetPPODataStart = FALSE;
    if (pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType] < (PG_PER_SLC_BLK - 1))
    {
        usCurTargetPPO = pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType];
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucLunOrder[ucLunInSuperPu] = ucLunInSuperPu;
            ulLunOrderTS[ucLunInSuperPu] = INVALID_8F;
            ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
            pTargetRPMT = (RPMT*)L2_TB_GetDataAddr(PAGE_TYPE_RPMT, ucSuperPuNum, 0, (U16)ucTargetType);

            if (usCurTargetPPO == pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
            {                    
                /* record target block PPO - 1 TS */                
                ulLunOrderTS[ucLunInSuperPu] = pTargetRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[usCurTargetPPO - 1];
            }
            else if ((usCurTargetPPO - 1) != pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
            {
                DBG_Printf("[%s]SPU %d Lun %d PPO %d and Targettype %d PPO %d un-sync\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu,
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex], ucTargetType,usCurTargetPPO);
                DBG_Getch();
            }

            if (ulLunOrderTS[ucLunInSuperPu] == 0)
                bTargetPPODataStart = TRUE;
        }

        if (bTargetPPODataStart == FALSE)
        {
            /* there is no LunOrderTS = 0 data, drop all PPO -1 data and re-adjust target block PPO */
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                DBG_Printf("Lun %d LunOrderTS 0x%x\n", ucLunOrder[ucLunInSuperPu], ulLunOrderTS[ucLunInSuperPu]);
                ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
                if (usCurTargetPPO == pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
                {
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] -= 1;
                }
            }

            pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType] -= 1;
            DBG_Printf("[%s]no LunOrderTS 0, other data drop, adjust target PPO to %d\n", __FUNCTION__, pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType]);
        }
        else
        {
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("Before sort\n");
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                DBG_Printf("LunOrder[%d] = %d\n",ucLunInSuperPu, ucLunOrder[ucLunInSuperPu]);
            }

			for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                DBG_Printf("LunOrderTS[%d] = 0x%x\n",ucLunInSuperPu, ulLunOrderTS[ucLunInSuperPu]);
            }
#endif
            /* sort LunOrder by LunOderTS */
            L2_TB_Rebuild_SortLunOrderTS((U32 *)&ulLunOrderTS[0], (U32 *)&ucLunOrder[0]);

#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("after sort\n");
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                DBG_Printf("LunOrder[%d] = %d\n",ucLunInSuperPu, ucLunOrder[ucLunInSuperPu]);
            }
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                DBG_Printf("LunOrderTS[%d] = 0x%x\n",ucLunInSuperPu, ulLunOrderTS[ucLunInSuperPu]);
            }
#endif
            ucFirstNotSeqIndex = L2_TB_Rebuild_CheckOrdering((U32 *)&ulLunOrderTS[0], (U32 *)&ucLunOrder[0]);
            if (ucFirstNotSeqIndex != INVALID_2F)
            {
                DBG_Printf("need adjust PPO, 1stnotSeqindex %d and Lun %d\n", ucFirstNotSeqIndex, ucLunOrder[ucFirstNotSeqIndex]);
                L2_TB_Rebuild_ReAdjustTargetPPO(ucSuperPuNum, ucTargetType, (U32 *)&ulLunOrderTS[0], (U32 *)&ucLunOrder[0], ucFirstNotSeqIndex);
            }
        }
    }
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_DataResumeTargetBlockInfo(U8 ucSuperPuNum)
{
    U8  ucTargetType, ucTargetIndex, ucLunInSuperPu;
    U16 uVirBlk, uPhyBlk;
    U32 ulDataAddr;
    RPMT *pTargetRPMT;
    BOOL bResumeOk, bNeedErase = FALSE;
    U32 TargetOffsetBitMap[TARGET_ALL - 1] = {0};
    U16 MaxPPO[TARGET_ALL - 1] = { 0 };
    U16 MinPPO[TARGET_ALL - 1];
    U16 usTempPPO;
    RPMT* pRPMT;

    for (ucTargetType = 0; ucTargetType < (TARGET_ALL - 1); ucTargetType++)
    {
        bResumeOk = FALSE;
        TargetOffsetBitMap[ucTargetType] = 0;
        MinPPO[ucTargetType] = INVALID_4F;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
            if (INVALID_4F != pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
            {
                pPBIT[ucSuperPuNum]->m_TargetBlock[ucTargetType] = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];

                DBG_Printf("[%s] Index %d Target %d Lun %d TargetBlk 0x%x TargetPPO %d\n", __FUNCTION__, ucTargetIndex, ucTargetType, ucLunInSuperPu,
                    pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex],
					pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex]);

                if (pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] > MaxPPO[ucTargetType])
                {
                    MaxPPO[ucTargetType] = pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
                    pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType] = MaxPPO[ucTargetType];
                }

                /* recored min PPO */
                if (pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] < MinPPO[ucTargetType])
                {
                    MinPPO[ucTargetType] = pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
                }

                /* need copy RPMT buffer to pointer 0 */
                if ((0 != ucTargetIndex) && (ucTargetType != TARGET_TLC_WRITE))
                {
                    g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType] = ucTargetIndex;
                    pTargetRPMT = (RPMT*)L2_TB_GetDataAddr(PAGE_TYPE_RPMT, ucSuperPuNum, ucLunInSuperPu, (U16)ucTargetType);

                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] = pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][1];
                    /* reset RPMT buffer pointer to 0 */
                    pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType] = 0;
                    g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType] = 0;

                    ulDataAddr = L2_TB_GetDataAddr(PAGE_TYPE_RPMT, ucSuperPuNum, ucLunInSuperPu, (U16)ucTargetType);
                    COM_MemCpy((U32 *)ulDataAddr, (U32 *)pTargetRPMT, sizeof(RPMT_PER_LUN) / sizeof(U32));
                }
                else
                {
                    if (0 == ucTargetIndex && 1 == g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType])
                    {
                        g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType] = 0;
                    }
                }

                if (bResumeOk == FALSE)
                {
                    bResumeOk = TRUE;
                }
            }
        }

        DBG_Printf("Target type %d: MaxPPO %d MinPPO %d\n", ucTargetType, MaxPPO[ucTargetType], MinPPO[ucTargetType]);

        /* check whether Max PPO and Min PPO distance is 1. If not, use Min PPO to adjust distance = 1 between Max PPO and Min PPO*/
        if (((MaxPPO[ucTargetType] - MinPPO[ucTargetType]) > 1) && (bResumeOk == TRUE) && (MinPPO[ucTargetType] != INVALID_4F))
        {
            DBG_Printf("[%s]target type %d Max PPO %d and Min PPO %d distance is over 1\n", __FUNCTION__, ucTargetType, MaxPPO[ucTargetType], 
				MinPPO[ucTargetType]);

            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
                if (pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] > (MinPPO[ucTargetType] + 1))
                {
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = MinPPO[ucTargetType] + 1;
                }
            }

            pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType] = MinPPO[ucTargetType] + 1;
            DBG_Printf("[%s]Final adjust PPO = %d\n", __FUNCTION__, pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType]);
        }

        /* Check whether each Lun OrderTS of target block PPO is  sequential, if not, adjust it to sequential */
        L2_TB_Rebuild_CheckIsTargetPPODataSeq(ucSuperPuNum, ucTargetType);

        /* After adjusting PPO, target block PPO may be 0. FullRecovery should release current target block and erase this block */
        if (pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType] == 0)
        {
            uVirBlk = pPBIT[ucSuperPuNum]->m_TargetBlock[ucTargetType];
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                uPhyBlk = pVBT[ucSuperPuNum]->m_VBT[uVirBlk].PhysicalBlockAddr[ucLunInSuperPu];
                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = uPhyBlk;
            }

            /* Neither of HW or SLC GC target block will happen 1 target block PPO is 1 and page 0 data is not sequential? */
            bResumeOk = FALSE;
            bNeedErase = TRUE;
        }
        else
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];

                if (pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType] == pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
                {
                    TargetOffsetBitMap[ucTargetType] |= (1 << ucLunInSuperPu);

                    if (SUPERPU_LUN_NUM_BITMSK == TargetOffsetBitMap[ucTargetType])
                    {
                        TargetOffsetBitMap[ucTargetType] = 0;
                    }
                }
                else
                {
                    usTempPPO = pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];

                    if (0 == usTempPPO)
                    {
                        DBG_Printf("Reset SPU %d LUN %d Target %d PPO %d RPMT TS = INVALID_8F\n", ucSuperPuNum, ucLunInSuperPu, ucTargetType, usTempPPO);
                        g_PuInfo[ucSuperPuNum]->m_pRPMT[ucTargetType][ucTargetIndex]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usTempPPO] = INVALID_8F;
                    }
                }
            }
			
            pPBIT[ucSuperPuNum]->m_TargetOffsetBitMap[ucTargetType] = TargetOffsetBitMap[ucTargetType];

            DBG_Printf("[%s] TargetType %d TargetOffsetBitMap 0x%x VBN %d RPMTBufPointer %d\n", __FUNCTION__,
                ucTargetType, TargetOffsetBitMap[ucTargetType], pPBIT[ucSuperPuNum]->m_TargetBlock[ucTargetType], g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[ucTargetType]);
        }
        
        if(bResumeOk == FALSE)
        {
            pPBIT[ucSuperPuNum]->m_TargetBlock[ucTargetType] = INVALID_4F;
            pPBIT[ucSuperPuNum]->m_TargetPPO[ucTargetType] = INVALID_4F;
        }
    }

    /*Reset PuInfo Page0's RPMT(SuperPageTS/LunOrderTS) of RPMTPointer1, because Adjust DirtyCnt will use it */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pRPMT = g_PuInfo[ucSuperPuNum]->m_pRPMT[TARGET_HOST_WRITE_NORAML][1];
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[0] = INVALID_8F;
        pRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[0] = INVALID_8F;
    }

    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
    return bNeedErase;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataCheckSameVirBlock(RED* pRed, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;
    U16 usVirBlk;
    L2_TB_REBUILD_STATUS NextStatus;
    U8 ucTargetType;
    U8 ucTargetIndex;
    BOOL bSearched = FALSE;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    usVirBlk = pRed->m_RedComm.bsVirBlockAddr;
    ucTargetType = pRed->m_RedComm.bcBlockType - BLOCK_TYPE_SEQ;

    /* search for current block PPO */
    for (ucTargetIndex = 0; ucTargetIndex < TEMP_BLK_CNT_PER_TARGET; ucTargetIndex++)
    {
        if (usVirBlk == pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
        {
            bSearched = TRUE;
            break;
        }
    }

    /* previous block found was a target block, need erase the block with less ppo */
    if (TRUE == bSearched)
    {
        /* Second PhyBlk has same VirBlk with first PhyBlk,second phyBlk should also read red from Pg0~PPO,
            at last use PPO larger phyblk as TargetBlk,otherwise,if PPO equal,use UECCCnt less block as TargetBlk */
        pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
        pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu] = usVirBlk;

        NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
    }
    else
    {
        /* previous block found was a closed block, just erase current block */
        pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
        NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
    }

    return NextStatus;
}

/*rebuild Err TLC block  VBT and PBIT */
void MCU1_DRAM_TEXT L2_TB_ErrTLC_Handle(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usErrTLCBlkCnt;
    U32 i;
    U32 VirtualBlockAddr;
    U32 PhyBlockAddr;

    //get err tlc cnt 
    usErrTLCBlkCnt = pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu];

    for (i = 0; i < usErrTLCBlkCnt; i++)
    {
        PhyBlockAddr = pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i];

        if (INVALID_4F == PhyBlockAddr)
            continue;

        /*skip TLCGC RPMTErrBlk*/
        if (INVALID_4F != pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].VirtualBlockAddr)
            continue;

        //search VBT to find a free item
        for (VirtualBlockAddr = 0; VirtualBlockAddr < TLC_BLK_CNT; VirtualBlockAddr++)
        {
            if (FALSE != pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bPhyAddFindInFullRecovery)
            {
                continue;
            }

            break;
        }

        //Set LastTimeSatmp
        pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].BlockType = BLOCK_TYPE_TLC_W;
        pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][PhyBlockAddr].PageType = PAGE_TYPE_DATA;
        pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][PhyBlockAddr].OPType = OP_TYPE_GC_WRITE;
        //atterntion, rebuild must Special process for LastPageTimeSatmp is 0, will not be cliped for rebuild
        pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][PhyBlockAddr].LastPageTimeStamp = 0;

        DBG_Printf("[%s]SPU %d Lun %d last page UECC TLC Blk new VBlk %d ==> PhyBlk %d\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, PhyBlockAddr);

        L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, VirtualBlockAddr, FALSE, BLOCK_TYPE_TLC_W);
    }
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataTLCNextPage(RED* pRed, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 i;
    U16 usBlock;
    U16 usVirBlockAddr;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlockType;
    L2_TB_REBUILD_STATUS NextStatus;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    PageType = pRed->m_RedComm.bcPageType;
    BlockType = pRed->m_RedComm.bcBlockType;
    usVirBlockAddr = pRed->m_RedComm.bsVirBlockAddr;

    //Not WL case
    if (FALSE == (pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].bPhyAddFindInFullRecovery & (0x1 << ucLunInSuperPu)))
    {
        /* set PBIT and VBT */
        pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usBlock].LastPageTimeStamp = 0;
        L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlockAddr, FALSE, BlockType);

        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
    else //WL case ,special process 
    {
        L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);

        pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;

        for (i = 0; i < pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu]; i++)
        {
            if (usBlock == pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i])
            {
                /*Remove Block from ErrTLCBlk Que*/
                pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i] = INVALID_4F;
            }
        }
        DBG_Printf("DataTLCNextPage WL Case,SPU %d LUN %d VirBlk %d erase PhyBlk %d, Old TLC Blk %d Not Free \n", ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr, usBlock,
            pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].PhysicalBlockAddr[ucLunInSuperPu]);
        NextStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
    }
    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataTLCLastPage(RED* pRed, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U8 i;
    U16 usBlock;
    U16 usOldBlock;
    U16 usVirBlockAddr;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlockType;
    L2_TB_REBUILD_STATUS NextStatus;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    PageType = pRed->m_RedComm.bcPageType;
    BlockType = pRed->m_RedComm.bcBlockType;
    usVirBlockAddr = pRed->m_RedComm.bsVirBlockAddr;

    if (PAGE_TYPE_RPMT != PageType)
    {
        DBG_Printf("L2_TB_Rebuild_DataTLCLastPage invalid PageType 0x%x\n", PageType);
        DBG_Getch();
    }

    //Not WL case
    if (FALSE == (pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].bPhyAddFindInFullRecovery & (0x1 << ucLunInSuperPu)))
    {
        /* set PBIT and VBT */
        L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlockAddr, FALSE, BlockType);

        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
    else //WL case ,special process 
    {
        usOldBlock = pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].PhysicalBlockAddr[ucLunInSuperPu];
        
        for (i = 0; i < pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu]; i++)
        {
            if (usOldBlock == pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i])
            {
                pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i] = INVALID_4F;
                break;
            }
        }

        /*Old bounding phyblock isn't a TLC RPMTUECC BLK*/
        if (i >= pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu])
        {
            /*TWO TLC SWL block all closed, just erase later one*/
            L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);

            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            DBG_Printf("DataTLCLastPage WL Case,SPU %d LUN %d VirBlk %d PhyBlk %d, Old TLC Blk %d Not Free \n", ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr, usBlock, pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].PhysicalBlockAddr[ucLunInSuperPu]);
            NextStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
        }
        else//Old bounding phyblock is a TLC RPMTUECC BLK
        {
#ifdef SIM
            if (BLOCK_TYPE_TLC_GC != BlockType)
            {
                DBG_Printf("Only TLCGC RPMTUECC block will bounding VBN&PBN befor L2_TB_ErrTLC_Handle");
                DBG_Getch();
            }
#endif
            /*relieve old mapping use curren mapping*/
            //L2_TB_Rebuild_DataBlockSetFree
            L2_TB_Rebuild_DataBlockRollBackTLCSetAllocated(ucSuperPuNum, ucLunInSuperPu, usOldBlock, usVirBlockAddr);
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usOldBlock;

            /* set PBIT and VBT */
            L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlockAddr, FALSE, BlockType);

            NextStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
        }
    }
    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataFirstPage(RED* pRed, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;
    U16 usVirBlk;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlockType;
    L2_TB_REBUILD_STATUS NextStatus;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    PageType = pRed->m_RedComm.bcPageType;
    BlockType = pRed->m_RedComm.bcBlockType;
    usVirBlk = pRed->m_RedComm.bsVirBlockAddr;

    if (PAGE_TYPE_DATA != PageType)
    {
        DBG_Printf("L2_TB_Rebuild_DataFirstPage invalid PageType 0x%x, Lun%d VBN%d PBN%d pg%d blkType%d, optype%d\n", PageType, ucLunInSuperPu, 
			pRed->m_RedComm.bsVirBlockAddr, usBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu], BlockType, pRed->m_RedComm.eOPType);
		DBG_Printf("table end blk %d, data blk start %d\n", g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu], g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu]);
        DBG_Getch();
    }

    //Not WL case
    if (FALSE == (pVBT[ucSuperPuNum]->m_VBT[usVirBlk].bPhyAddFindInFullRecovery & (0x1 << ucLunInSuperPu)))
    {
        /* set PBIT and VBT */
        L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlk, pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu], BlockType);

        NextStatus = L2_TB_REBUILD_READ_LAST_PAGE;
    }
    else  //WL case, special process
    {
        //SLC blk
        if (TRUE == pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu])
        {
            /* blakezhang: need to add errorhandling code */
            NextStatus = L2_TB_Rebuild_DataCheckSameVirBlock(pRed, ucSuperPuNum, ucLunInSuperPu);
        }
        else
        {
            DBG_Printf("[%s]:SPU%d Lun%d SLC case may not occur TLC case\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
            DBG_Getch();

        }
    }
    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataFirstPageEmpty(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    L2_TB_REBUILD_STATUS NextStatus;

    /* check 2nd page empty for data block */
    pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu] = TRUE;
    NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataFirstPageUECC(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;
    L2_TB_REBUILD_STATUS NextStatus;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

    /* check next page for data block */
    //pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = TRUE;
    NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;

#ifdef SIM
    L2_CollectErrAddrOfCheckPMT(ucSuperPuNum, usBlock, 0);
#endif
    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataCheckLastPage(RED* pRed, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    PAGE_TYPE PageType;
    L2_TB_REBUILD_STATUS NextStatus;

    PageType = pRed->m_RedComm.bcPageType;

    /* NOTICE: data block last page is RPMT */
    if (PAGE_TYPE_RPMT == PageType)
    {
        /* just finish */
        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }
    else
    {
        DBG_Printf("[%s]SPU%d Lun%d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, PageType);
        DBG_Getch();
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataLastPageEmpty(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    L2_TB_REBUILD_STATUS NextStatus;

    /* could be Target Block - reset Page index and start to check next Page */
    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
    NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataCheckNextPage(RED* pRed, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;
    U16 usVirBlockAddr;
    U8  ucTargetType;
    U8  ucTargetIndex;
    PAGE_TYPE PageType;
    BLOCK_TYPE BlockType;
    L2_TB_REBUILD_STATUS NextStatus;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    PageType = pRed->m_RedComm.bcPageType;
    BlockType = pRed->m_RedComm.bcBlockType;

    if (PAGE_TYPE_DATA == PageType)
    {
        if (TRUE == pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu])
        {
            /* this 2nd data page and 1st Page Empty */
            //L2_Set_TableBlock_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
           // pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
#if 0
            usVirBlockAddr = pRed->m_RedComm.bsVirBlockAddr;
            if (FALSE != (pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].bPhyAddFindInFullRecovery & (0x1 << ucLunInSuperPu)))
            {
                /* blakezhang: need to add errorhandling code */
                NextStatus = L2_TB_Rebuild_DataCheckSameVirBlock(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
            else
            {
                /* set PBIT and VBT */
                L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlockAddr,
                    pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu], BlockType);
                NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
            }
#endif
        }
        else if (TRUE == pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu])
        {
            /* all previous Page UECC */
            L2_Set_TableBlock_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;

            usVirBlockAddr = pRed->m_RedComm.bsVirBlockAddr;
            if (FALSE != (pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].bPhyAddFindInFullRecovery & (0x1 << ucLunInSuperPu)))
            {
                /* blakezhang: need to add errorhandling code */
                NextStatus = L2_TB_Rebuild_DataCheckSameVirBlock(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
            else
            {
                /* set PBIT and VBT */
                L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlockAddr, 
                   pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu], BlockType);
                NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
            }
        }
        else if (TRUE == pTBRebManagement->ucFirstPageBlkTypeErr[ucSuperPuNum][ucLunInSuperPu])
        {
            /* handle first page read success but block type is error case: need to record VBN and check whether there has the same 
               VBN */
            L2_Set_TableBlock_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
            pTBRebManagement->ucFirstPageBlkTypeErr[ucSuperPuNum][ucLunInSuperPu] = FALSE;

            usVirBlockAddr = pRed->m_RedComm.bsVirBlockAddr;
            if (FALSE != (pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].bPhyAddFindInFullRecovery & (0x1 << ucLunInSuperPu)))
            {
                /* blakezhang: need to add errorhandling code */
                NextStatus = L2_TB_Rebuild_DataCheckSameVirBlock(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
            else
            {
                /* set PBIT and VBT */
                L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlockAddr, 
                    pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu], BlockType);
                NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
                DBG_Printf("[%s]SPU %d Lun %d 1stpg BlkTypeErr blk %d find correct RED: blktype %d, pgtype %d, optype %d and VBN %d\n", 
                    __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, usBlock, pRed->m_RedComm.bcBlockType, pRed->m_RedComm.bcPageType, 
                    pRed->m_RedComm.eOPType, usVirBlockAddr);
            }
        }
        else if (INVALID_4F != pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu])
        {
            /* current block has more Pages than previous block - search for current block PPO then erase previous block */
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
    }
    else if (PAGE_TYPE_RPMT == PageType)
    {
        /* last page, just finish */
        if (TRUE == pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu])
        {
            L2_Set_TableBlock_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;

            usVirBlockAddr = pRed->m_RedComm.bsVirBlockAddr;

            if (FALSE != (pVBT[ucSuperPuNum]->m_VBT[usVirBlockAddr].bPhyAddFindInFullRecovery & (0x1 << ucLunInSuperPu)))
            {
                /* blakezhang: need to add errorhandling code */
                NextStatus = L2_TB_Rebuild_DataCheckSameVirBlock(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
            else
            {
                /* set PBIT and VBT */
                L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, usVirBlockAddr, 
                    pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu], BlockType);
                NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
            }
        }
        else if (TRUE == pTBRebManagement->ucFirstPageBlkTypeErr[ucSuperPuNum][ucLunInSuperPu])
        {
            /* RPMT read success, but all SLC data pages are ERROR block type, need erase */
            if (pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu] == pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu])
            {
                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                DBG_Printf("FirstPgBlkTypeErr SPU %d Blk %d BlkTypeErrCnt %d, all block dummy data, erase!\n", ucSuperPuNum, usBlock, BlockType, 
                    pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum]);
                NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
            }
        }
        else if (INVALID_4F != pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu])
        {
            /* update data then erase previous block */
            usVirBlockAddr = pRed->m_RedComm.bsVirBlockAddr;
            ucTargetType = pRed->m_RedComm.bcBlockType - BLOCK_TYPE_SEQ;
            for (ucTargetIndex = 0; ucTargetIndex < TEMP_BLK_CNT_PER_TARGET; ucTargetIndex++)
            {
                if (usVirBlockAddr == pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
                {
                    pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = INVALID_4F;
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = INVALID_4F;
                    break;
                }
            }

            /* it is error if can not search the same VirBlock on this target */
            if (TEMP_BLK_CNT_PER_TARGET == ucTargetIndex)
            {
                DBG_Printf("[%s]SPU %d Lun %d invalid TargetIndex 0x%x\n", __FUNCTION__,ucSuperPuNum, ucLunInSuperPu, ucTargetIndex);
                DBG_Getch();
            }

            usBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr);
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            L2_TB_Rebuild_DataBlockSwap(ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr, usBlock, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);

            NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
        }
        else
        {
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
    else
    {
        DBG_Printf("[%s]SPU %d Lun %d invalid PageType 0x%x\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, PageType);
        DBG_Getch();
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataNextPageEmpty(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16  usBlock;
    U16  usVirBlockAddr;
    BLOCK_TYPE BlockType;
    U8 ucTargetType;
    U8 ucTargetIndex;
    BOOL bSearched = FALSE;
    L2_TB_REBUILD_STATUS NextStatus;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    if (TRUE == pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu])
    {
        if (FALSE == pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu])
        {
            DBG_Printf("[%s]SPU %d Lun %d TLC will not read next page empty, er\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            NextStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
            //DBG_Getch();
        }
        else
        {
            /* 2nd page also Empty Page, free block */
            L2_TB_Rebuild_DataBlockSetFree(ucSuperPuNum, ucLunInSuperPu, usBlock);
            pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu] = FALSE;

            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
    else if (TRUE == pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu])
    {
        if (FALSE == pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu])
        {
            DBG_Printf("[%s]SPU %d Lun %d TLC will not read next page UECC, er\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
			pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            NextStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
            //DBG_Getch();
        }
        else
        {
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
			DBG_Printf("[%s]SLC: L %d PB %d 1stpg UECC, next pg empty, er\n", __FUNCTION__, ucLunInSuperPu, usBlock);//
        }
    }
    else if (TRUE == pTBRebManagement->ucLastPageUECC[ucSuperPuNum][ucLunInSuperPu])
    {
        /* erase this block due to erase SPOR */
        /* Erase SPOR cases:
           1. All pages UECC.
           2.Page 0 retry pass, page 1~N UECC, page N + 1 empty, last page UECC.
           3.Page(0, 2, 3, 4, 502 бн) without pair page : read pass, low pages(with pair page) UECC and upper page read pass. */

        pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
        pTBRebManagement->ucLastPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
        pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
        NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
        DBG_Printf("[%s]LastPageUECC Erase SPU %d Lun %d VirBlk %d\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, usBlock);
    }
    else if (TRUE == pTBRebManagement->ucFirstPageBlkTypeErr[ucSuperPuNum][ucLunInSuperPu])
    {
        /* L3 error handling move target block before, but all page are dummy. It needs to be erase. */
        if (pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu] == pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu])
        {
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
            DBG_Printf("FirstPgBlkTypeErr next pg empty SPU %d Blk %d ErrCnt %d, erase\n", ucSuperPuNum, usBlock, pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu]);
        }
    }
    else if (INVALID_4F != pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu])
    {
        /* update data then erase block */
        usVirBlockAddr = pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu];
        usBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr);
        BlockType = pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].BlockType;
        ucTargetType = BlockType - BLOCK_TYPE_SEQ;

        /* must scan a open block on this target before */
        if (usVirBlockAddr != pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0])
        {
            DBG_Printf("SPU %d Lun %d VirBlk 0x%x WriteType %d\n", ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr, ucTargetType);
            DBG_Getch();
        }
        else
        {
            if (pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] > pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu])
            {
                /* just erase current block */
                DBG_Printf("SPU %d Fullrecovery SameVirBlk 0x%x (PhyBlk1 0x%x PPO 0x%x > PhyBlk2 0x%x PPO 0x%x)\n", ucSuperPuNum, 
                    pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu], usBlock, pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0],
                    pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);

                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            }
            else if (pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] == pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu])
            {
                DBG_Printf("SPU %d Fullrecovery SameVirBlk 0x%x (PhyBlk1 0x%x PPO 0x%x UECCErrCnt1 %d == PhyBlk2 0x%x PPO 0x%x UECCErrCnt2 %d)\n", ucSuperPuNum, 
                    pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu], usBlock, pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0], 
                    pTBRebManagement->usTargetBlkUECCErrCnt[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0], pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu], pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu]);

                if (pTBRebManagement->usTargetBlkUECCErrCnt[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] > pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu])
                {
                    /* erase previous block */
                    pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                    pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
                    pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] = INVALID_4F;
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] = INVALID_4F;

                    /* current TargetIndex must be 1 */
                    if (1 != pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType])
                    {
                        DBG_Getch();
                    }

                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][1] = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                    pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][1] = usVirBlockAddr;
                    L2_TB_Rebuild_DataBlockSwap(ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr, usBlock, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);
                    L2_TB_Rebuild_DataSaveTargetRPMT(ucSuperPuNum, ucLunInSuperPu, ucTargetType, 1);
                }
                else
                {
                    pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                    pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
                }

            }
            else
            {
                /* erase previous block */
                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
                pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] = INVALID_4F;
                pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] = INVALID_4F;

                /* current TargetIndex must be 1 */
                if (1 != pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType])
                {
                    DBG_Getch();
                }

                pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][1] = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][1] = usVirBlockAddr;
                L2_TB_Rebuild_DataBlockSwap(ucSuperPuNum, ucLunInSuperPu, usVirBlockAddr, usBlock, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);
                L2_TB_Rebuild_DataSaveTargetRPMT(ucSuperPuNum, ucLunInSuperPu, ucTargetType, 1);
            }

            NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
			DBG_Printf("[%s]SLC: L %d same VB, PB %d er\n", __FUNCTION__, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu]);//
        }
    }
    else
    {
        /* record Target Block and PPO */
        BlockType = pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].BlockType;
        if ((BlockType < BLOCK_TYPE_SEQ) || (BlockType >= BLOCK_TYPE_EMPTY))
        {
            DBG_Printf("[%s]SPU0%d Lun%d BlockType0x%x ERROR!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, BlockType);
            DBG_Getch();
        }
        else
        {
            ucTargetType = BlockType - BLOCK_TYPE_SEQ;
			DBG_Printf("[%s]Find Target blk, type %d\n", __FUNCTION__, BlockType);
            ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
            pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].VirtualBlockAddr;
            pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
            pTBRebManagement->usTargetBlkUECCErrCnt[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu];

            L2_TB_Rebuild_DataSaveTargetRPMT(ucSuperPuNum, ucLunInSuperPu, ucTargetType, ucTargetIndex);
        }

        DBG_Printf("[%s]Lun %d VB:%d PB:%d is OpenBlk,TargetType %d, TargetIndex %d\n", __FUNCTION__, ucLunInSuperPu,
            pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].VirtualBlockAddr, usBlock, ucTargetType, ucTargetIndex);

        NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
    }

    return NextStatus;
}

L2_TB_REBUILD_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_DataNextPageUECC(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16  usBlock;
    U16  usPagePerBlk;
    L2_TB_REBUILD_STATUS NextStatus;

    usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
    if (TRUE == pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu])
    {
        usPagePerBlk = PG_PER_SLC_BLK;
    }
    else
    {
        // TLC will not read next page, so this case will not happen
        DBG_Printf("[%s]SPU%d Lun%d TLC doesn't need to read next page\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
        DBG_Getch();
        //usPagePerBlk = PG_PER_SLC_BLK; //nick delete
    }

    if (TRUE == pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu])
    {
        /* next page UECC Page, go on reading */
        if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] < (usPagePerBlk - 1))
        {
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            /* last page - erase current block */
            pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
			DBG_Printf("[%s]SLC: L %d PB %d 1stpg empty, next pg UECC, er\n", __FUNCTION__, ucLunInSuperPu, usBlock);//
        }
    }
    else if (TRUE == pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu])
    {
        /* go on reading */
        if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] < (usPagePerBlk - 1))
        {
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            /* last page - erase current block */
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
            NextStatus = L2_TB_REBUILD_ERASE_BLOCK;
   		    DBG_Printf("[%s]SLC: L %d PB %d 1stpg UECC, next pg UECC, er\n", __FUNCTION__, ucLunInSuperPu, usBlock);//
        }
    }
    else
    {
        /* next page UECC Page, go on reading */
        if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] < (usPagePerBlk - 1))
        {
            NextStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
        }
        else
        {
            /* last page UECC - just finish */
            NextStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }

    return NextStatus;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_BackupBlkInfo(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U32 SLCBlkend, TLCBlkEnd;

    L2_BbtGetSlcTlcBlock(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), &SLCBlkend, &TLCBlkEnd);

    //must process later, read it form bbt  #! Addisonsu
    g_PuInfo[ucSuperPuNum]->m_SLCEndPhyBlk[ucLunInSuperPu] = SLCBlkend;
    g_PuInfo[ucSuperPuNum]->m_TLCStartPhyBlk[ucLunInSuperPu] = TLCBlkEnd;
}

BOOL MCU1_DRAM_TEXT L2_TB_Is_TLCBlk(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 uPhyBlk)
{
    if (uPhyBlk >= g_PuInfo[ucSuperPuNum]->m_TLCStartPhyBlk[ucLunInSuperPu])
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_ErrTLCHandle(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    L2_TB_ERRTLC_HANDLE_STATUS ErrTLCHandleStatus;
    U32 usErrTLCBlkCnt, ulErrTLCHandleSN;
    U32 i;
    U32 uErrTlcPhyBlk;
    U8* pTheFirstThreePageStatus;
    U32 TLCPageTime;
    U32 VirtualBlockAddr, ErrTLCBlkVir;
    U32 usPhySlcBlk;
    U32 CurrTS;
    BOOL bOpenTLCFlag = TRUE;
	BOOL bTLCGCNotCloseFlag = FALSE;
    BOOL bWLFlag = FALSE;
    RED *pRed;
    L2_TB_REBUILD_STATUS *pStatus = &(pTBRebManagement->RebuildDataBlockStatus[ucSuperPuNum][ucLunInSuperPu]);
    PhysicalAddr FlashAddr = { 0 };
    U16 usBlock;

    ErrTLCHandleStatus = pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu];
    usErrTLCBlkCnt = pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu];
    ulErrTLCHandleSN = pTBRebManagement->ulErrTLCHandleSN[ucSuperPuNum][ucLunInSuperPu];

    switch (ErrTLCHandleStatus)
    {
    case L2_TB_ERRTLC_HANDLE_START:
    {
        if (ulErrTLCHandleSN >= usErrTLCBlkCnt)
        {
            *pStatus = L2_TB_REBUILD_SUCCESS;
        }

        for (i = 0; i < PG_PER_WL; i++)
        {
            pTBRebManagement->ulTheFirstThreePageTime[ucSuperPuNum][ucLunInSuperPu][i] = 0;
            pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i] = SUBSYSTEM_STATUS_INIT;
            pTBRebManagement->usThreeSrcSlcBlk[ucSuperPuNum][ucLunInSuperPu][i] = INVALID_4F;
        }

        pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_READ;
    }
        break;
    case L2_TB_ERRTLC_HANDLE_READ:
    {
        uErrTlcPhyBlk = pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][ulErrTLCHandleSN];

        /*Skip earsed RPMTUECC ErrTlcBlk*/
        if (INVALID_4F == uErrTlcPhyBlk)
        {
            pTBRebManagement->ulErrTLCHandleSN[ucSuperPuNum][ucLunInSuperPu]++;
            pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_START;
            break;
        }

        FlashAddr.m_PUSer = ucSuperPuNum;
        FlashAddr.m_BlockInPU = uErrTlcPhyBlk;
        FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;

        //get not send cmd page
        for (i = 0; i < PG_PER_WL; i++)
        {
            if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                break;

            if (SUBSYSTEM_STATUS_INIT != pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i])
                continue;
            FlashAddr.m_PageInBlock = i*(PG_PER_SLC_BLK - 1);

            pTheFirstThreePageStatus = &(pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i]);
            pRed = pTBRebManagement->ulTheFirstThreePageRed[ucSuperPuNum][ucLunInSuperPu][i];

            L2_LoadSpare(&FlashAddr, (U8*)pTheFirstThreePageStatus, (U32 *)pRed, FALSE);
        }

        //all page send to L3, go to next stage 
        if (PG_PER_WL == i)
        {
            pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_WAIT;
        }
    }
        break;
    case L2_TB_ERRTLC_HANDLE_WAIT:
    {
        if (SUBSYSTEM_STATUS_PENDING == pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][2])
            break;

        //all finished
        for (i = 0; i < PG_PER_WL; i++)
        {
            if (SUBSYSTEM_STATUS_EMPTY_PG == pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i])
            {
                // TLC has empty page, it may be SPOR during erasing, erase directly
                break;
            }
            else if (SUBSYSTEM_STATUS_SUCCESS != pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i] &&
                SUBSYSTEM_STATUS_RECC != pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i] &&
                SUBSYSTEM_STATUS_RETRY_SUCCESS != pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i])
            {
                /*1 TLC page UECC, source block information may be lost, remove from ErrTLC queue */
                pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i] = INVALID_4F;
                break;
            }

            pRed = (pTBRebManagement->ulTheFirstThreePageRed[ucSuperPuNum][ucLunInSuperPu][i]);
            //get the page time
            pTBRebManagement->ulTheFirstThreePageTime[ucSuperPuNum][ucLunInSuperPu][i] = pRed->m_RedComm.ulTimeStamp;
        }

        //Err case not hanlde
        if (PG_PER_WL != i)
        {
            if (pTBRebManagement->ucTheFirstThreePageStatus[ucSuperPuNum][ucLunInSuperPu][i] == SUBSYSTEM_STATUS_EMPTY_PG)
            {
                pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_ERASE;
            }
            else
            {
                pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_NEXTBLK;
            }
            break;
        }
        else
        {
            pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_FINDSLCBLK;
        }

        /*check TLCGC case*/
        for (i = 0; i < PG_PER_WL; i++)
        {
            pRed = (pTBRebManagement->ulTheFirstThreePageRed[ucSuperPuNum][ucLunInSuperPu][i]);
            if (BLOCK_TYPE_TLC_GC == pRed->m_RedComm.bcBlockType)
            {
                usBlock = pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][ulErrTLCHandleSN];
                ErrTLCBlkVir = pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].VirtualBlockAddr;
                //find TLCGC SrcBlk
                for (VirtualBlockAddr = 0; VirtualBlockAddr < TLC_BLK_CNT; VirtualBlockAddr++)
                {
                    /* TLC last page UECC block doesn't need to join check */
                    if (ErrTLCBlkVir == VirtualBlockAddr)
                        continue;

                    if (FALSE != (pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bPhyAddFindInFullRecovery & (1 << ucLunInSuperPu)))
                    {
                        usPhySlcBlk = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr);
                        CurrTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usPhySlcBlk].LastPageTimeStamp;

                        //WL case, 
                        if ((CurrTS == pRed->m_DataRed.ulTLCGC1stSrcBlockTS)
                            && (VirtualBlockAddr == pRed->m_DataRed.bsTLCGC1stSrcBlock))
                        {
                            bTLCGCNotCloseFlag = TRUE;
                            break;
                        }
                    }
                }

                if (bTLCGCNotCloseFlag)
                {
                    DBG_Printf("TLCGC Case all SrcBlk not erase, Pu %d PhyBlk%d_VirBlk%d need erase\n", ucSuperPuNum, usBlock, VirtualBlockAddr);
                    //FIRMWARE_LogInfo("WL Case,Pu %d PhyBlk %d need erase\n", ucPuNum, usBlock);

                    L2_TB_Rebuild_DataBlockRollBackTLCSetAllocated(ucSuperPuNum, ucLunInSuperPu, usBlock, L2_PBIT_GetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usBlock));

                    pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_ERASE;
                    break;
                }
            }
            else if ((BLOCK_TYPE_SEQ == pRed->m_RedComm.bcBlockType) || (BLOCK_TYPE_RAN == pRed->m_RedComm.bcBlockType))
            {
                /* current TLC block is TLC merge block, go to check SLC source block directly */
                break;
            }
        }
    }
        break;
    case L2_TB_ERRTLC_HANDLE_NEXTBLK:
        pTBRebManagement->ulErrTLCHandleSN[ucSuperPuNum][ucLunInSuperPu] ++;
        pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_START;
        break;

    case  L2_TB_ERRTLC_HANDLE_FINDSLCBLK:
        usBlock = pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][ulErrTLCHandleSN];
        //First, if WL case    
        TLCPageTime = pTBRebManagement->ulTheFirstThreePageTime[ucSuperPuNum][ucLunInSuperPu][0];

        for (i = 0; i < PG_PER_WL; i++)
        {
            TLCPageTime = pTBRebManagement->ulTheFirstThreePageTime[ucSuperPuNum][ucLunInSuperPu][i];

            //find src slc  blk
            for (VirtualBlockAddr = TLC_BLK_CNT; VirtualBlockAddr < VIR_BLK_CNT; VirtualBlockAddr++)
            {
                if (FALSE != (pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bPhyAddFindInFullRecovery & (1 << ucLunInSuperPu)))
                {
                    usPhySlcBlk = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr);
                    CurrTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usPhySlcBlk].TimeStamp;

                    //find
                    if (CurrTS == TLCPageTime)
                    {
                        pTBRebManagement->usThreeSrcSlcBlk[ucSuperPuNum][ucLunInSuperPu][i] = usPhySlcBlk;
                        break;
                    }
                }
            }
        }

        bOpenTLCFlag = TRUE;
        for (i = 0; i < PG_PER_WL; i++)
        {
            if (INVALID_4F == pTBRebManagement->usThreeSrcSlcBlk[ucSuperPuNum][ucLunInSuperPu][i])
            {
                bOpenTLCFlag = FALSE;
                break;
            }
        }

        if (bOpenTLCFlag)
        {
            /* three Src SLC blocks are find, current TLC block can be erased */
            uErrTlcPhyBlk = pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][ulErrTLCHandleSN];
            pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_ERASE;
            DBG_Printf("SPU %d Lun %d ErrTLCBlk %d, three src phy blk is %d_%d_%d, can be erased\n", ucSuperPuNum, ucLunInSuperPu, uErrTlcPhyBlk,
                pTBRebManagement->usThreeSrcSlcBlk[ucSuperPuNum][ucLunInSuperPu][0],
                pTBRebManagement->usThreeSrcSlcBlk[ucSuperPuNum][ucLunInSuperPu][1],
                pTBRebManagement->usThreeSrcSlcBlk[ucSuperPuNum][ucLunInSuperPu][2]);
        }
        else
        {
            /* any Src SLC block isn't found, current TLC block cannot be erased */
            DBG_Printf("SPU %d Lun %d ErrTLCBlk %d any src cannot find\n", ucSuperPuNum, ucLunInSuperPu, usBlock);
            pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_NEXTBLK;
        }
        break;
    case L2_TB_ERRTLC_HANDLE_ERASE:
        usBlock = pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][ulErrTLCHandleSN];
        /* erase current data block */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
			L2_EraseTLCBlock(ucSuperPuNum, ucLunInSuperPu, usBlock, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), FALSE);

            /* set current block free */
            L2_TB_Rebuild_DataBlockSetFree(ucSuperPuNum, ucLunInSuperPu, usBlock);

            L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usBlock);

			ErrTLCBlkVir = L2_PBIT_GetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usBlock, INVALID_4F);
            L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, ErrTLCBlkVir, 0);

            /* clear corresponding bit of Lun in bPhyAddFindInFullRecovery */
			pVBT[ucSuperPuNum]->m_VBT[ErrTLCBlkVir].bPhyAddFindInFullRecovery &= ~(1 << ucLunInSuperPu);
            //pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].BlockType = BLOCK_TYPE_EMPTY;
            DBG_Printf("ERRHAND TLCBlk SPU %d Lun %d PhyBlk %d BlkType %d\n", ucSuperPuNum, ucLunInSuperPu, usBlock,
                pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].BlockType);

            //Remove this err tlc blk
            pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][ulErrTLCHandleSN] = INVALID_4F;
            pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_NEXTBLK;
        }
        break;
    default:
        break;
    
    }
}

U32 MCU1_DRAM_TEXT L2_TB_Rebuild_DataBlock(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usBlock;
    U32 ulRet;
    U8  ucFlashStatus;
    RED *pRed;
    L2_TB_REBUILD_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };
    BOOL bSLCMode, bTLCMode = FALSE;

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBRebManagement->RebuildDataBlockStatus[ucSuperPuNum][ucLunInSuperPu]);

    switch (*pStatus)
    {
    case L2_TB_REBUILD_START:
    {
        /* rebuild tlc start physcal blk and slc end physical info */
        L2_TB_Rebuild_BackupBlkInfo(ucSuperPuNum, ucLunInSuperPu);

        /* reset max TS */
        L2_TB_ResetDataBlockMaxTS(ucSuperPuNum, ucLunInSuperPu);

        /* locate data block base */
        pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu];

        bTLCMode = L2_TB_Is_TLCBlk(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);
        pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu] = !(bTLCMode);
        *pStatus = L2_TB_REBUILD_READ_FIRST_PAGE;
    }
        //break;

    case L2_TB_REBUILD_READ_FIRST_PAGE:
    {
        /* read 1st page redundant of each block */
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            ;
        }
        else if (TRUE == pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError)
        {
            /* finish for bad block */
            *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
        else
        {
            /* update page index to 1st page */
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;

            /* get flashaddr, temp red and send read request to L3 */
            FlashAddr.m_PUSer = ucSuperPuNum;
            FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
            FlashAddr.m_BlockInPU = usBlock;
            FlashAddr.m_PageInBlock = 0;
            FlashAddr.m_LPNInPage = 0;

            /* add double check,because FlashAddr.m_BlockInPU may not have enough bits. */
            if (FlashAddr.m_BlockInPU != pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu])
            {
                DBG_Printf("[%s]SPU%d Lun%d m_BlockInPU(0x%x) != usBlockIndex(0x%x) \n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu,
                    FlashAddr.m_BlockInPU, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);
                DBG_Getch();
            }

            bSLCMode = pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu];
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

            //FIRMWARE_LogInfo("ReadFirstPage SPU %d LUN %d Blk %d Pg %d SLC %d\n", ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], FlashAddr.m_PageInBlock, bSLCMode);
            L2_LoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, bSLCMode);
            *pStatus = L2_TB_REBUILD_CHECK_FIRST_PAGE;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_FIRST_PAGE:
    {
        /* wait for flash idle */
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
        bTLCMode = !(pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu]); //L2_TB_Is_TLCBlk(ucPuNum, usBlock);

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            if (TRUE == bTLCMode)
            {
                /* Update tlc first page time, but tlc block correct TS is in last page RED, so read last page */
                pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usBlock].TimeStamp = pRed->m_RedComm.ulTimeStamp;
                *pStatus = L2_TB_REBUILD_READ_LAST_PAGE;
            }
            else
            {
                /* set SLC block PBIT information */
                L2_Set_TableBlock_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], pRed);

                /* may be caused by L3 error handling Copydata or TLC merge read UECC page + RED */ // need double check which operation RED block type is ERROR
                if ((BLOCK_TYPE_ERROR == pRed->m_RedComm.bcBlockType) && (0 == pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]))
                {
                    pTBRebManagement->ucFirstPageBlkTypeErr[ucSuperPuNum][ucLunInSuperPu] = TRUE;
                    pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu]++;
                    DBG_Printf("FirstPgBlkTypeErr SPU %d Blk %d\n", ucSuperPuNum, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);
                    *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
                    break;
                }

                L2_TB_Rebuild_DataSetRPMT(ucSuperPuNum, ucLunInSuperPu, pRed); // RPMT need to double check
               
                //Update cur blk max timestamp
                L2_TB_UpdateCurBlockMaxTS(ucSuperPuNum, ucLunInSuperPu, pRed);

                //FIRMWARE_LogInfo("CheckFirstPage SPU %d LUN %d Blk %d Success\n", ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);

                //Rebuild PBIT & VBT by first page redundant info
                *pStatus = L2_TB_Rebuild_DataFirstPage(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            if (bTLCMode)
            {
                /* set current block free */
                L2_TB_Rebuild_DataBlockSetFree(ucSuperPuNum, ucLunInSuperPu, usBlock);

                L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
                *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
            }
            else
            {
                /* rebuild read 1st page EMPTY_PG */
                *pStatus = L2_TB_Rebuild_DataFirstPageEmpty(ucSuperPuNum, ucLunInSuperPu);
            }

            //FIRMWARE_LogInfo("CheckFirstPage SPU %d LUN %d Blk %d EmptyPage\n", ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);
        }
        else
        {
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = TRUE;

            if (FALSE == bTLCMode)
            {
                /* SLC block rebuild read 1st page UECC */
                *pStatus = L2_TB_Rebuild_DataFirstPageUECC(ucSuperPuNum, ucLunInSuperPu);
                pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu]++;
            }
            else  //TLC Block speciall process            
            {
                *pStatus = L2_TB_REBUILD_READ_LAST_PAGE;
            }
        }
    }
        break;

    case L2_TB_REBUILD_READ_LAST_PAGE:
    {
        /* read last page redundant of each block */
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            /* update page index to last page */
            bSLCMode = pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu];

            if (TRUE == bSLCMode)
            {
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = (PG_PER_SLC_BLK - 1);
            }
            else  //TLC BLK
            {
                //pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = (PG_PER_SLC_BLK*PG_PER_WL - 1);
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = (PG_PER_TLC_BLK - 1);      
            }

            /* get flashaddr, temp red and send read request to L3 */
            FlashAddr.m_PUSer = ucSuperPuNum;
            FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
            FlashAddr.m_BlockInPU = usBlock;
            FlashAddr.m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
            FlashAddr.m_LPNInPage = 0;

#ifdef SIM
            /* add double check,because FlashAddr.m_PageInBlock may not have enough bits. */
            if (FlashAddr.m_PageInBlock != pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu])
            {
                DBG_Printf("[%s]SPU %d m_PageInBlock(0x%x) != usPageIndex(0x%x) \n",
                    __FUNCTION__, ucSuperPuNum, FlashAddr.m_PageInBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);
                DBG_Getch();
            }
#endif
            //FIRMWARE_LogInfo("ReadLastPage SPU %d LUN %d Blk %d Pg %d SLC %d\n", ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 
            //    FlashAddr.m_PageInBlock, bSLCMode);

            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            L2_LoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, bSLCMode);
            *pStatus = L2_TB_REBUILD_CHECK_LAST_PAGE;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_LAST_PAGE:
    {
        /* wait for flash idle */
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
        bTLCMode = !(pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu]);//L2_TB_Is_TLCBlk(ucPuNum, usBlock);

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            L2_TB_UpdateCurBlockMaxTS(ucSuperPuNum, ucLunInSuperPu, pRed);

            if (0 == pRed->m_RedComm.ulTimeStamp)
            {
                DBG_Printf("[%s]SPU%d LUN%d Phyblk%d last page TS = 0!!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, usBlock);
                DBG_Getch();
            }

            //FIRMWARE_LogInfo("CheckLastPage SPU %d LUN %d Blk %d Success,VirBlk %d LastPageTS %d\n", ucSuperPuNum, ucLunInSuperPu, 
            //    pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], pRed->m_RedComm.bsVirBlockAddr, pRed->m_RedComm.ulTimeStamp);

            //TLC special process
            if (bTLCMode)
            {
                //update tlc PBIT info
                L2_Set_TableBlock_PBIT_Info_TLC(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
                //Update last page timestamp
                //L2_TB_UpdateCurBlockMaxTS(ucSuperPuNum, ucLunInSuperPu, pRed);

                *pStatus = L2_TB_Rebuild_DataTLCLastPage(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
            else
            {
                *pStatus = L2_TB_Rebuild_DataCheckLastPage(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            //FIRMWARE_LogInfo("CheckLastPage SPU %d LUN %d Blk %d EmptyPage\n", ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]);

            //TLC Blk special process
            if (bTLCMode)
            {
                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                DBG_Printf("SPU %d LUN %d PhyBlk %d last page empty(TLC openblk),erase later\n", ucSuperPuNum, ucLunInSuperPu, usBlock);
                *pStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
            }
            else
            {
                /* rebuild read last page EMPTY_PG */
                *pStatus = L2_TB_Rebuild_DataLastPageEmpty(ucSuperPuNum, ucLunInSuperPu);
            }
        }
        else  //Last page err
        {
            //FIRMWARE_LogInfo("Pu %d PhyBlk %d LastPage Err\n", ucPuNum, usBlock);

            //Not TLC blk
            if (FALSE == bTLCMode)
            {
                /* rebuild read last page UECC - reset PageIndex and check next page */
                pTBRebManagement->ucLastPageUECC[ucSuperPuNum][ucLunInSuperPu] = TRUE;//nick add, original code has
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
                pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu]++;
                *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
            }
            else //TLC blk special process
            {
                U16 usErrTLCBlkCnt;

                DBG_Printf("TLCBlk RPMT UECC PU %d LUN %d BLK %d PG %d\n", ucSuperPuNum, ucLunInSuperPu, usBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);
                //statistic err TLC blk
                usErrTLCBlkCnt = pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu];
                pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][usErrTLCBlkCnt] = usBlock;
                pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu]++;
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = PG_PER_BLK - 2; //read TLC second RPMT page 1534
                *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;

                DBG_Printf("SuperPU %d LUN %d PhyBlk 0x%x TLC LastPage UECC\n", ucSuperPuNum, ucLunInSuperPu, usBlock);
            }
        }
    }
        break;

    case L2_TB_REBUILD_READ_NEXT_PAGE:
    {
        /* read next page redundant of each block */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            bSLCMode = pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu];
            if (TRUE == bSLCMode)
            {
                /* update page index to next page */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]++;
            }

            /* get flashaddr, temp red and send read request to L3 */
            FlashAddr.m_PUSer = ucSuperPuNum;
            FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
            FlashAddr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
            FlashAddr.m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
            FlashAddr.m_LPNInPage = 0;

            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            L2_LoadSpare(&FlashAddr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                (U32 *)pRed, pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu]);

            //FIRMWARE_LogInfo("ReadNextPage SPU %d LUN %d Blk %d Pg %d SLC %d\n", ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu],
            //    FlashAddr.m_PageInBlock, pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu]);

            if ((FALSE == bSLCMode) && (pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE))
            {
                /* TLC block all RPMT UECC, update page index to next page read page start from zero */
                pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]++;
            }
            *pStatus = L2_TB_REBUILD_CHECK_NEXT_PAGE;

        }
    }
        break;

    case L2_TB_REBUILD_CHECK_NEXT_PAGE:
    {
        /* wait for flash idle */
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
        usBlock = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
        bSLCMode = pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
        {
            pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
            if (TRUE == bSLCMode)
            {               
                /* may be caused by L3 UECC Copydata */
                if (BLOCK_TYPE_ERROR == pRed->m_RedComm.bcBlockType)
                {
                    pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu]++;
					/* RPMT and all SLC data pages are ERROR block type, need erase */
                    if (pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu] == PG_PER_SLC_BLK)
                    {
                        pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                        DBG_Printf("SPU %d Lun %d Blk %d ErrCnt %d, all blk dummy, erase\n", ucSuperPuNum, ucLunInSuperPu, usBlock,
                            pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu]);
                        *pStatus = L2_TB_REBUILD_ERASE_BLOCK;
						break;
                    }

                    *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
					break;
                }
                L2_TB_Rebuild_DataSetRPMT(ucSuperPuNum, ucLunInSuperPu, pRed);
                L2_TB_UpdateCurBlockMaxTS(ucSuperPuNum, ucLunInSuperPu, pRed);

                *pStatus = L2_TB_Rebuild_DataCheckNextPage(pRed, ucSuperPuNum, ucLunInSuperPu);
            }
            else
            {
                /*not TLCGC BLK no need to read anymore, as DataPage's VBN info isn't reliable*/
                if (BLOCK_TYPE_TLC_GC == pRed->m_RedComm.bcBlockType)
                {
                    L2_Set_TableBlock_PBIT_Info_TLC(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
                    *pStatus = L2_TB_Rebuild_DataTLCNextPage(pRed, ucSuperPuNum, ucLunInSuperPu);
#ifdef SPOR_DEBUGLOG_ENABLE
                    if (pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] == FALSE)
                    {
                        DBG_Printf("SPU%d Lun%d TLC GC blk%d last page UECC case, read RPMT%d success, VBN%d\n", ucSuperPuNum, ucLunInSuperPu,
                            usBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu], pRed->m_RedComm.bsVirBlockAddr);
                    }
                    else
                    {
                        DBG_Printf("SPU%d Lun%d TLC GC blk%d all RPMT UECC case, read next page%d success, VBN%d\n", ucSuperPuNum, ucLunInSuperPu,
                            usBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu], pRed->m_RedComm.bsVirBlockAddr);
                    }
#endif
                }
                else if (BLOCK_TYPE_ERROR == pRed->m_RedComm.bcBlockType)
                {
                    /*if TLCSWL SrcBlk DataPage UECC, then It's BlockType will filled with BLOCK_TYPE_ERROR, then write to target*/
                    *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
                }         
                else if (BLOCK_TYPE_TLC_W == pRed->m_RedComm.bcBlockType)
                {
                    /* Read TLC merge block RPMT case will enter here */
					if (pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] == FALSE)
                    {
                        L2_Set_TableBlock_PBIT_Info_TLC(ucSuperPuNum, ucLunInSuperPu, usBlock, pRed);
                        *pStatus = L2_TB_Rebuild_DataTLCNextPage(pRed, ucSuperPuNum, ucLunInSuperPu);
#ifdef SPOR_DEBUGLOG_ENABLE
                        DBG_Printf("SPU%d Lun%d TLC merge blk%d last page UECC case, read RPMT%d success, VBN%d\n", ucSuperPuNum, ucLunInSuperPu,
                            usBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu], pRed->m_RedComm.bsVirBlockAddr);
#endif
					}
                    else
                    {
                        DBG_Printf("SPU%d Lun%d TLC merge blk%d all RPMT UECC case must not enter here\n", ucSuperPuNum, ucLunInSuperPu, usBlock);
                        DBG_Getch();
                    }
                }
                else if (BLOCK_TYPE_SEQ == pRed->m_RedComm.bcBlockType || BLOCK_TYPE_RAN == pRed->m_RedComm.bcBlockType)
                {
                    /* TLC merge block first page read success, not need to read next page to find the VBN mapping */
                    if (pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE)
                    {
                        *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;                    
                    }
                    else
                    {
                        DBG_Printf("SPU%d Lun%d TLC merge blk%d all RPMT UECC, but all RPMT UECC flag not TRUE\n", ucSuperPuNum, ucLunInSuperPu, usBlock);
                        DBG_Getch();
                    }
                }
                else
                {
                    DBG_Printf("SPU%d Lun%d TLC blk%d read next page%d block type %d error\n", ucSuperPuNum, ucLunInSuperPu,
                        usBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu], pRed->m_RedComm.bcBlockType);
                    DBG_Getch();
                }
            }
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
        {
            if (FALSE == bSLCMode)
            {
                DBG_Printf("TLC blk All RPMT UECC will read next page, it cannot be empty!");
                pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                *pStatus  = L2_TB_REBUILD_ERASE_TLC_BLOCK;
				break;
                //DBG_Getch();
            }
            /* rebuild next page EMPTY_PG */
            *pStatus = L2_TB_Rebuild_DataNextPageEmpty(ucSuperPuNum, ucLunInSuperPu);
          
        }
        else
        {
            /* If UECC page count of current data block is over threshold, erase directly, it may be SPOR during erase. */            
            pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu]++;
            if (TRUE == bSLCMode)
            {
                if (pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu] >= SLC_UECCCNT_ERASE_THRESHOLD)
                {
                    DBG_Printf("Lun%d SLC PB%d UECC Cnt %d over threshold, erase\n", ucLunInSuperPu, usBlock,
                        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu]);
     	            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                    *pStatus = L2_TB_REBUILD_ERASE_BLOCK;
                    break;
                }
            }
            else
            {
                if (pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu] >= TLC_UECCCNT_ERASE_THRESHOLD)
                {
                    DBG_Printf("Lun%d TLC PB%d UECC Cnt %d over threshold, erase\n", ucLunInSuperPu, usBlock,
                        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu]);
     	            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                    *pStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
                    break;
                }
            }

            if ((FALSE == bSLCMode) && (pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] == TRUE))
            {
                /*TLC block all RPMT UECC */
                if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] < (PG_PER_BLK - 1))
                {
                    *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
                }
                else
                {
                    /* TLC block all page UECC, need erase */
                    pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = usBlock;
                    *pStatus = L2_TB_REBUILD_ERASE_TLC_BLOCK;
                }
                break;
            } 
            else if((FALSE == bSLCMode) && (pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] == FALSE))
            {
                DBG_Printf("TLC RPMT UECC, read PU %d LUN %d BLK %d PG %d to find valid VBN mapping\n", ucSuperPuNum, ucLunInSuperPu, usBlock, pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);
                /* TLC block last page UECC case, read page 1534 and 1533 RPMT to find VBN in RPMT page */
                if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] > (PG_PER_BLK - 3))
                {
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]--;
                    *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
                }
                else
                {
                    /* TLC merge block last WL RPMT page all UECC, read page 0 to check current tlc block type is GC or merge*/
					pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] = TRUE;
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
                    *pStatus = L2_TB_REBUILD_READ_NEXT_PAGE;
					DBG_Printf("SPU%d Lun%d TLC block%d all RPMT UECC, need read next page from page 0\n", ucSuperPuNum,
                        ucLunInSuperPu, usBlock);
                }
                break;
            }
            /* SLC block rebuild next page UECC */		
            *pStatus = L2_TB_Rebuild_DataNextPageUECC(ucSuperPuNum, ucLunInSuperPu);
        }
    }
        break;

    case L2_TB_REBUILD_CURRENT_BLOCK_FINISH:
    {
        pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]++;
        if (pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] >= (BLK_PER_PLN + RSV_BLK_PER_PLN))    //All Finish
        {
            if (pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu] != 0)
                *pStatus = L2_TB_REBUILD_ERRTLC_HANDLING;
            else
                *pStatus = L2_TB_REBUILD_SUCCESS;
        }
        else
        {
            *pStatus = L2_TB_REBUILD_READ_FIRST_PAGE;
        }

        pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu] = (TRUE == L2_TB_Is_TLCBlk(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu])) ? (FALSE) : (TRUE);
        pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu] = 0;
		pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu] = 0;
        pTBRebManagement->ucLastPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
		pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;

        /* update Max TS */
        L2_TB_UpdateDataBlockMaxTS(ucSuperPuNum, ucLunInSuperPu);
        L2_TB_ResetCurBlockMaxTS(ucSuperPuNum, ucLunInSuperPu);

        //Init FirstPageUECC
        pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
        pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu] = FALSE;
		pTBRebManagement->ucFirstPageBlkTypeErr[ucSuperPuNum][ucLunInSuperPu] = FALSE;
    }
        break;

    case L2_TB_REBUILD_ERASE_BLOCK:
    {
        /* erase current data block */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            if (INVALID_4F == pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu])
            {
                DBG_Printf("[%s]SPU%d Lun%d ERASE Block ERROR!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                DBG_Getch();
            }
            else
            {
                /* send read request to L3 */
                if (pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] == pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu])
                {
                    L2_TB_ResetCurBlockMaxTS(ucSuperPuNum, ucLunInSuperPu);
                }

                L2_FtlEraseBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu],
					&(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), FALSE, pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu], FALSE);
                *pStatus = L2_TB_REBUILD_WAIT_ERASE_BLOCK;
#ifdef SPOR_DEBUGLOG_ENABLE
                DBG_Printf("SLC: Lun %d PBN %d E S\n", ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu]);//
#endif
            }
        }
    }
        break;

    case L2_TB_REBUILD_WAIT_ERASE_BLOCK:
    {
        /* wait for flash idle */
        usBlock = pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu];
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
        {
            /* set current block free */
            L2_TB_Rebuild_DataBlockSetFree(ucSuperPuNum, ucLunInSuperPu, usBlock);// need double check
            L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usBlock);
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
#ifdef SPOR_DEBUGLOG_ENABLE
			DBG_Printf("SLC: Lun %d PBN %d E D\n", ucLunInSuperPu, usBlock);//
#endif
        }
        else
        {
            /* erase fail - save BBT and and set current block error */
            //L2_TB_Rebuild_DataBlockSetFree(ucSuperPuNum, ucLunInSuperPu, usBlock);// need double check

            L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usBlock, FALSE);
            //L2_PBIT_Set_Allocate(ucSuperPuNum, ucLunInSuperPu, usBlock, FALSE);
            L2_PBIT_Set_Error(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
            L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_BbtAddBbtBadBlk(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), usBlock, RT_BAD_BLK, ERASE_ERR);

            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
            *pStatus = L2_TB_REBUILD_ERASE_SAVE_BBT;
#ifdef SPOR_DEBUGLOG_ENABLE
			DBG_Printf("SLC: Lun %d PBN %d E F\n", ucLunInSuperPu, usBlock);//
#endif
        }
    }
        break;

    case L2_TB_REBUILD_ERASE_TLC_BLOCK:
    {
        /* erase current data block */
        if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            if (INVALID_4F == pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu])
            {
                DBG_Printf("[%s]SPU%d Lun%d ERASE Block ERROR!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                DBG_Getch();
            }
            else
            {
                /* send read request to L3 */
                if (pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] == pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu])
                {
                    L2_TB_ResetCurBlockMaxTS(ucSuperPuNum, ucLunInSuperPu);
                }

                L2_EraseTLCBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu],
					&(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), FALSE);
                *pStatus = L2_TB_REBUILD_WAIT_ERASE_TLC_BLOCK;
#ifdef SPOR_DEBUGLOG_ENABLE
				DBG_Printf("TLC: Lun %d PBN %d E S\n", ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu]);//
#endif
            }
        }
    }
        break;
    case L2_TB_REBUILD_WAIT_ERASE_TLC_BLOCK:
    {
        /* wait for flash idle */
        usBlock = pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu];
        ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
        {
            ;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
        {
            /* set current block free */
            L2_TB_Rebuild_DataBlockSetFree(ucSuperPuNum, ucLunInSuperPu, usBlock);

            L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usBlock);

            //pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].BlockType = BLOCK_TYPE_TLC_W;
            DBG_Printf("TLC OpenBlk SPU %d Lun %d PhyBlk %d BlockType %d\n", ucSuperPuNum, ucLunInSuperPu, usBlock,
                pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].BlockType);

            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
        else
        {
            /* erase fail - save BBT and and set current block error */
            L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usBlock, FALSE);
            L2_PBIT_Set_Allocate(ucSuperPuNum, ucLunInSuperPu, usBlock, FALSE);
            L2_PBIT_Set_Error(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
            L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usBlock);
            L2_BbtAddBbtBadBlk(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), usBlock, RT_BAD_BLK, ERASE_ERR);
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
            *pStatus = L2_TB_REBUILD_ERASE_SAVE_BBT;
#ifdef SPOR_DEBUGLOG_ENABLE
			DBG_Printf("TLC: Lun %d PBN %d E F\n", ucLunInSuperPu, usBlock);//
#endif
        }
    }
        break;

    case L2_TB_REBUILD_ERASE_SAVE_BBT:
    {
        /* save BBT for root pointers */
        if (TRUE == L2_BbtSave(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), INVALID_2F))
        {
            gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
            *pStatus = L2_TB_REBUILD_CURRENT_BLOCK_FINISH;
        }
    }
        break;

    case  L2_TB_REBUILD_ERRTLC_HANDLING:
        L2_TB_Rebuild_ErrTLCHandle(ucSuperPuNum, ucLunInSuperPu);

        break;

    case L2_TB_REBUILD_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_REBUILD_SUCCESS:
    {
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_Rebuild_DataBlock invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_IsCurVirBlkTarget(U8 ucSuperPuNum, U16 usVirBlk)
{
    U8 ucTargetIndex, ucLunInSuperPu;
    TargetType ucTargetType;

    for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
            if (usVirBlk == pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
            {
                return TRUE;
            }
        }

    }

    return FALSE;
}

U8 MCU1_DRAM_TEXT L2_TB_Rebuild_CalMissMappingLunCnt(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, ucMissMappingLunCnt = 0;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (ulLunBlk[ucLunInSuperPu] == INVALID_4F)
        {
            ucMissMappingLunCnt++;
        }
    }

	return ucMissMappingLunCnt;
}

/* This function finds first no mapping Lun and first has mapping Lun in current VBN */
void MCU1_DRAM_TEXT L2_TB_Rebuild_GetFirstNoMappingLun(U8 ucSuperPuNum, U8* NoMappingLun, U8* FirstHasMappingLun)
{
    U8 ucLunInSuperPu;
    BOOL bNoMappingLunFind = FALSE, bFirstHasMappingLunFind = FALSE;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (ulLunBlk[ucLunInSuperPu] == INVALID_4F && bNoMappingLunFind == FALSE)
        {
            bNoMappingLunFind = TRUE;
            *NoMappingLun = ucLunInSuperPu;
        }
        else if (ulLunBlk[ucLunInSuperPu] != INVALID_4F && bFirstHasMappingLunFind == FALSE)
        {
            bFirstHasMappingLunFind = TRUE;
           *FirstHasMappingLun = ucLunInSuperPu;
        }

		if ((bNoMappingLunFind == TRUE) && (bFirstHasMappingLunFind == TRUE))
            break;
    }
}


BOOL MCU1_DRAM_TEXT L2_TB_RebuildFindTSinErrTLCBlk(U8 ucSuperPuNum, U16 usVirBlk)
{
    U8 ucMissMappingLunCnt, ucCurFinishLunCnt;
    U8 ucLunInSuperPu,NoMappingLun, FirstHasMappingLun;
    U16 i, usErrTLCBlkCnt;
    U16 usPhyBlk, PhyBlockAddr;
    U32 FirstPageTS, ErrTLCBlkTS;
    BOOL bAllMapping = TRUE, bFind;

    ucMissMappingLunCnt = L2_TB_Rebuild_CalMissMappingLunCnt(ucSuperPuNum);
    ucCurFinishLunCnt = 0;
    /* search miss Lun mapping in ErrTLCBlk data structure */
    while (ucMissMappingLunCnt > ucCurFinishLunCnt)
    {
        L2_TB_Rebuild_GetFirstNoMappingLun(ucSuperPuNum, &NoMappingLun, &FirstHasMappingLun);
        usPhyBlk = ulLunBlk[FirstHasMappingLun];
        FirstPageTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[FirstHasMappingLun][usPhyBlk].TimeStamp;
        usErrTLCBlkCnt = pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][NoMappingLun];
        bFind = FALSE;

        for (i = 0; i < usErrTLCBlkCnt; i++)
        {
            PhyBlockAddr = pTBRebManagement->usErrTLCBlk[ucSuperPuNum][NoMappingLun][i];

            if (INVALID_4F == PhyBlockAddr)
                continue;

            if (INVALID_4F != pPBIT[ucSuperPuNum]->m_PBIT_Entry[NoMappingLun][PhyBlockAddr].VirtualBlockAddr)
                continue;

            ErrTLCBlkTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[NoMappingLun][PhyBlockAddr].TimeStamp;

            if (FirstPageTS == ErrTLCBlkTS)
            {
                ulLunBlk[NoMappingLun] = PhyBlockAddr;
                pPBIT[ucSuperPuNum]->m_PBIT_Entry[NoMappingLun][PhyBlockAddr].BlockType = pPBIT[ucSuperPuNum]->m_PBIT_Entry[FirstHasMappingLun][usPhyBlk].BlockType;
                pPBIT[ucSuperPuNum]->m_PBIT_Info[NoMappingLun][PhyBlockAddr].PageType = pPBIT[ucSuperPuNum]->m_PBIT_Info[FirstHasMappingLun][usPhyBlk].PageType;
                pPBIT[ucSuperPuNum]->m_PBIT_Info[NoMappingLun][PhyBlockAddr].OPType = pPBIT[ucSuperPuNum]->m_PBIT_Info[FirstHasMappingLun][usPhyBlk].OPType;
                /* atterntion, rebuild must Special process for LastPageTimeSatmp is 0, will not be cliped for rebuild */
                pPBIT[ucSuperPuNum]->m_PBIT_Info[NoMappingLun][PhyBlockAddr].LastPageTimeStamp = 0;
                DBG_Printf("[%s]SPU%d VBN%d find miss mapping Lun%d, same TS 0x%x_0x%x ErrTLC block\n", __FUNCTION__, ucSuperPuNum, usVirBlk, NoMappingLun,
                    FirstPageTS, ErrTLCBlkTS);

                L2_TB_Rebuild_DataBlockSetAllocated(ucSuperPuNum, NoMappingLun, PhyBlockAddr, usVirBlk, FALSE, pPBIT[ucSuperPuNum]->m_PBIT_Entry[NoMappingLun][PhyBlockAddr].BlockType);
                ucCurFinishLunCnt++;
				bFind = TRUE;
                break;
            }
        }	

        if (bFind == FALSE)
        {
            /* Lun cannot find the correct mapping, but also need to add finish count */
            ucCurFinishLunCnt++;
            DBG_Printf("[%s]SPU%d Virblk%d Lun%d cannot find the same TS in ErrTLCBlk data structure\n", __FUNCTION__, ucSuperPuNum, usVirBlk, NoMappingLun);
        }
    }

	/* check whether all Lun of current Virblk have full PBN mapping */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (ulLunBlk[ucLunInSuperPu] == INVALID_4F)
        {
            bAllMapping = FALSE;
            break;
        }
    }

    return bAllMapping;
}

L2_TB_REBUILD_CHECK_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_CheckMappingNextStage(U8 ucSuperPuNum, U16 usVirBlk)
{
    U8 ucLunInSuperPu;
    BOOL bNeedEraseBlk = FALSE;
    L2_TB_REBUILD_CHECK_STATUS ucRet;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] != INVALID_4F)
        {
            bNeedEraseBlk = TRUE;
            break;
        }
    }

    if (bNeedEraseBlk == TRUE)
    {
        ucRet = L2_TB_REBUILD_CHECK_ERASE_BLOCK;
    }
    else
    {
        if (usVirBlk == VIR_BLK_CNT)
        {
            ucRet = L2_TB_REBUILD_CHECK_LOAD_TARGET_RPMT;
        }
        else
        {
            DBG_Printf("[%s]: no this case, bNeedEraseBlk%d, usVirBlk%d\n", __FUNCTION__, bNeedEraseBlk, usVirBlk);
            DBG_Getch();
        }
    }

    return ucRet;
}

L2_TB_REBUILD_CHECK_STATUS MCU1_DRAM_TEXT L2_TB_Rebuild_CheckEraseNextStage(U8 ucSuperPuNum, BOOL bNeedEraseAfterResume, BOOL bStartEraseInforHandling)
{
    L2_TB_REBUILD_CHECK_STATUS ucRet;

    if (bStartEraseInforHandling == TRUE)
    {
        if ((g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt + g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt) == 0)
        {
            ucRet = L2_TB_Rebuild_IsNeedSaveBBT(ucSuperPuNum);
        }
        else
        {
            ucRet = L2_TB_REBUILD_CHECK_HANLDE_ERASE_INFO;
        }
    }
    else if (bNeedEraseAfterResume == TRUE)
    {
        DBG_Printf("Erase done target block whose adjusted PPO is 0\n");
        if (bHandleSPORDuringErase[ucSuperPuNum] == TRUE)
            ucRet = L2_TB_REBUILD_CHECK_HANLDE_ERASE_INFO;
        else
            ucRet = L2_TB_REBUILD_CHECK_SUCCESS;
    }
    else
    {		
        if (pTBRebManagement->usBlockIndex[ucSuperPuNum][0] >= VIR_BLK_CNT)
        {
            ucRet = L2_TB_REBUILD_CHECK_LOAD_TARGET_RPMT;
        }
        else
        { 
            ucRet = L2_TB_REBUILD_CHECK_VBN_PBN_MAPPING;
        }
    }

    return ucRet;
}

#if 0
U32 MCU1_DRAM_TEXT L2_TB_RollBackEraseCount(void)
{
    U8  ucPuNum;
    U8  ucCheckPuCnt;
    U32 ulRet;
    U32 ulCmdRet;
    L2_TB_FULLRECOVERY_STATUS *pStatus;

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBManagement->FullRecoveryStatus);

    switch (*pStatus)
    {
    case L2_TB_FULLRECOVERY_START:
    {
        /* Init global values */
        L2_RT_Init();
        L2_PBIT_Init(FALSE, TRUE);
        L2_VBT_Init();

        L2_TB_ResetRebuildStatus();

        bTBRebuildPending = TRUE;
        *pStatus = L2_TB_FULLRECOVERY_REBUILD_TABLE;
    }
        break;

    case L2_TB_FULLRECOVERY_REBUILD_TABLE:
    {
        ucCheckPuCnt = 0;

        for (ucPuNum = 0; ucPuNum < SUBSYSTEM_PU_NUM; ucPuNum++)
        {
            ulCmdRet = L2_TB_Rebuild_TableBlock(ucPuNum);

            if (L2_TB_FAIL == ulCmdRet)
            {
                *pStatus = L2_TB_FULLRECOVERY_FAIL;
                break;
            }
            else if (L2_TB_SUCCESS == ulCmdRet)
            {
                ucCheckPuCnt++;
            }
        }

        if ((L2_TB_FAIL != ulCmdRet) && (ucCheckPuCnt >= SUBSYSTEM_PU_NUM))
        {
            *pStatus = L2_TB_FULLRECOVERY_SUCCESS;
        }
    }
        break;

    case L2_TB_FULLRECOVERY_FAIL:
    {
        L2_TB_ResetRebuildStatus();
        L2_TB_ResetFullRecoveryStatus();
        bTBRebuildPending = FALSE;
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_FULLRECOVERY_SUCCESS:
    {
        L2_TB_ResetRebuildStatus();
        L2_TB_ResetFullRecoveryStatus();
        bTBRebuildPending = FALSE;
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_RollBackEraseCount invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}
#endif

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_FindTargetBlkMapping(U8 ucSuperPuNum, U16 uTempVirNum, TargetType ucTargetType)
{
    U8 ucLunInSuperPu, ucTargetIndex;
    U8 ucNomappingCnt = 0;
    U16 usTargetPBN, usTargetVBN;
    BOOL ret = FALSE;
    PuInfo* pInfo;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (ulNeedFreeBlkTargetLun[ucLunInSuperPu] == FALSE)
            continue;

        ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
        usTargetVBN = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];

#ifndef SIM
        if (ucTargetIndex != 0)
        {
            DBG_Printf("[%s]SPU %d Lun %d blktype %d TargetIndex %d != 0 error!!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu,
                ucTargetType, ucTargetIndex);
            DBG_Getch();
        }

        if (usTargetVBN != INVALID_4F)
        {
            DBG_Printf("[%s]SPU %d Lun %d blktype %d TargetBlk %d !=INVALID_4F error!!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu,
                ucTargetType, ucTargetIndex);
            DBG_Getch();
        }
#endif
        
        usTargetPBN = pVBT[ucSuperPuNum]->m_VBT[uTempVirNum].PhysicalBlockAddr[ucLunInSuperPu];
        if (usTargetPBN != 0)
        {
            pInfo = g_PuInfo[ucSuperPuNum];
            /* Current no mapping Lun find target physical block in VBT */
            pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = uTempVirNum;
			ulLunBlk[ucLunInSuperPu] = uTempVirNum;
			pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = (PG_PER_SLC_BLK - 1);

            ulNeedFreeBlkTargetLun[ucLunInSuperPu] = FALSE;
            COM_BitMaskSet(&pInfo->m_RPMTFlushBitMap[ucTargetType], ucLunInSuperPu);
            DBG_Printf("[%s]:SPU %d VBN %d Lun %d find PBN mapping in VBT\n", __FUNCTION__, ucSuperPuNum, uTempVirNum, ucLunInSuperPu);
        }
        else
        {
            ucNomappingCnt++;
            DBG_Printf("[%s]:SPU %d VBN %d Lun %d no PBN mapping\n", __FUNCTION__, ucSuperPuNum, uTempVirNum, ucLunInSuperPu);
        }
    }  

    if (ucNomappingCnt == 0)
    {
        DBG_Printf("[%s]:SPU %d VBN %d targettype %d find all mapping in VBT\n", __FUNCTION__, ucSuperPuNum, uTempVirNum, ucTargetType);
        return TRUE;
    }
    else
    {
        DBG_Printf("[%s]:SPU %d VBN %d targettype %d some Lun need free PBN mapping\n", __FUNCTION__, ucSuperPuNum, uTempVirNum, ucTargetType);
		for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (ulNeedFreeBlkTargetLun[ucLunInSuperPu] == TRUE)
            {
                DBG_Printf("Lun %d need free PBN\n", ucLunInSuperPu);
            } 
		}
        return FALSE;
    }
}

/* check whether all Luns, which has VBN and PBN mapping, have the same VBN and TS,
    and allocate new physical block to no PBN target Lun */
void MCU1_DRAM_TEXT L2_TB_Rebuild_AllocateFreeBlkForLun(U8 ucSuperPuNum, U16 ulVirBlk, TargetType ucTargetType, U8 ucCurBlockType)
{
    U8 ucLunInSuperPu, ucTargetIndex;
    U16 PhyBlockAddr, LastPhyBlockAddr, usTargetVBN;
    BOOL bAllocPhyBlk;

    /* because only SLC block has the target block, we only need to allocate SLC range PBN to non-valid Lun */
    LastPhyBlockAddr = TABLE_BLOCK_COUNT;
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (ulNeedFreeBlkTargetLun[ucLunInSuperPu] == FALSE)
            continue;

        ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
#ifdef SIM
        if (ucTargetIndex != 0)
        {
            DBG_Printf("[%s]SPU%d Lun%d blocktype%d TargetIndex%d != 0 error!!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu,
                ucTargetType, ucTargetIndex);
            DBG_Getch();
        }
#endif
        bAllocPhyBlk = FALSE;
        for (PhyBlockAddr = LastPhyBlockAddr; PhyBlockAddr <= (g_PuInfo[ucSuperPuNum]->m_SLCEndPhyBlk[ucLunInSuperPu]); PhyBlockAddr++)
        {
            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == FALSE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bError == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bAllocated == TRUE)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].VirtualBlockAddr != INVALID_4F)
                continue;

            if (pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bFree == TRUE)
            {
                L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, ulVirBlk);
                L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, ulVirBlk, PhyBlockAddr);

                L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, FALSE);
                L2_PBIT_Set_Allocate(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, TRUE);

                L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, PhyBlockAddr, FALSE);
                pVBT[ucSuperPuNum]->m_VBT[ulVirBlk].bPhyAddFindInFullRecovery |= (0x1 << ucLunInSuperPu);
				//set PBIT block type
				pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].BlockType = ucCurBlockType;
                bAllocPhyBlk = TRUE;
                break;
            }
        }

        /*one Lun cannot find the free physical block, need getch. Enter here, it represents that  some Lun page 0 is empty and
                some Lun first page ahs data before abnormal shutdown. Thus, there must has the free phyblk can be allocated. */
        if (bAllocPhyBlk == FALSE)
        {
            DBG_Printf("[%s]SPU%d LUN%d no free phyblk can be allocated to target block\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
            DBG_Getch();
        }
        else
        {
            usTargetVBN = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
            if (usTargetVBN == INVALID_4F)
            {
                pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = ulVirBlk;
                ulLunBlk[ucLunInSuperPu] = ulVirBlk;
                pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = 0;
                ulNeedFreeBlkTargetLun[ucLunInSuperPu] = FALSE;
            }
            else
            {
                DBG_Printf("[%s]:SPU %d current Lun %d has found VBN %d during scan\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, ulVirBlk);
                DBG_Getch();
            }
        }
    }
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_SetEraseErrTargetBlk(U8 ucSuperPuNum, U8 ucErrLun, U8 ucTargetType, U8 ucTargetIndex)
{
    U16 PhyBlk;

    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] &= ~(1 << ucErrLun);
    PhyBlk = pVBT[ucSuperPuNum]->m_VBT[ulLunBlk[ucErrLun]].PhysicalBlockAddr[ucErrLun];
    pTBRebManagement->usBlockErase[ucSuperPuNum][ucErrLun] = PhyBlk;
    pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucErrLun][ucTargetType] = 0;
    pTBRebManagement->usTargetBlock[ucSuperPuNum][ucErrLun][ucTargetType][ucTargetIndex] = INVALID_4F;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_FindCorrectTargetBlk(U8 ucSuperPuNum, U8 ucTargetType)
{
    U8 ucLunInSuperPu, ucCorrectLun;
    U8 ucTargetIndex, ucCorrectTargetIndex;
    U16 uCorrectTargetVBN = INVALID_4F, uCorrectTargetPPO;

    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = SUPERPU_LUN_NUM_BITMSK;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
        if ((uCorrectTargetVBN == INVALID_4F) && (ulLunBlk[ucLunInSuperPu] != INVALID_4F))
        {
            ucCorrectLun = ucLunInSuperPu;
            uCorrectTargetVBN = ulLunBlk[ucLunInSuperPu];
            ucCorrectTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType]; 
            uCorrectTargetPPO = pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucCorrectTargetIndex];
            continue;
        }

        /* if target block of lun is invalid_4f, we don't need to check. Only target block of Lun is not invalid_4f and VBN is different, need check */
        if ((ulLunBlk[ucLunInSuperPu] != INVALID_4F) && (ulLunBlk[ucLunInSuperPu] != uCorrectTargetVBN))
        {
            ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];
            if (uCorrectTargetPPO > pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
            {
                DBG_Printf("[%s]SPU %d targettype %d correct VB %d Lun %d PPO %d, incorrect VB %d Lun %d PPO %d\n", __FUNCTION__, ucSuperPuNum,
                    ucTargetType, uCorrectTargetVBN, ucCorrectLun, uCorrectTargetPPO, ulLunBlk[ucLunInSuperPu], ucLunInSuperPu, 
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex]);
                L2_TB_Rebuild_SetEraseErrTargetBlk(ucSuperPuNum, ucLunInSuperPu, ucTargetType, ucTargetIndex);
            }
            else if (uCorrectTargetPPO < pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
            {
                DBG_Printf("[%s]SPU %d targettype %d correct VB %d Lun %d PPO %d, incorrect VB %d Lun %d PPO %d\n", __FUNCTION__, ucSuperPuNum,
                    ucTargetType, ulLunBlk[ucLunInSuperPu], ucLunInSuperPu, pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex],
                    uCorrectTargetVBN, ucCorrectLun, uCorrectTargetPPO);
                L2_TB_Rebuild_SetEraseErrTargetBlk(ucSuperPuNum, ucCorrectLun, ucTargetType, ucCorrectTargetIndex);

                /* update new target information */
                ucCorrectLun = ucLunInSuperPu;
                uCorrectTargetVBN = ulLunBlk[ucLunInSuperPu];
				ucCorrectTargetIndex = ucTargetIndex;
                uCorrectTargetPPO = pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
            }
            else
            {
                DBG_Printf("[%s]SPU %d targettype %d 2 Lun %d_%d VB %d_%d PPO %d_%d are the same error!\n", __FUNCTION__, ucSuperPuNum,
                    ucTargetType, ucCorrectLun, ucLunInSuperPu, uCorrectTargetVBN, ulLunBlk[ucLunInSuperPu], uCorrectTargetPPO,
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex]);
                DBG_Getch();
            }
        }
    }
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_CheckTargetBlock(U8 ucSuperPuNum, TargetType ucTargetType)
{
    U8 ucLunInSuperPu, ucTargetIndex, ucInvalidTargtCnt, ucCurBlockType;
    U16 uTempVirNum, uTempVirNum1, uPhyBlk;
    BOOL bCompareErr = FALSE;

    uTempVirNum = INVALID_4F;
    uTempVirNum1 = INVALID_4F;
    ucInvalidTargtCnt = 0;
    ucCurBlockType = INVALID_2F;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType];

        /* record each Lun target virtaul block number*/
        ulLunBlk[ucLunInSuperPu] = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];

        ulNeedFreeBlkTargetLun[ucLunInSuperPu] = FALSE;

        /* calculate no target LUN count , and check whether VBN and TS of valid Lun are the same*/
        if (ulLunBlk[ucLunInSuperPu] == INVALID_4F)
        {
            ulNeedFreeBlkTargetLun[ucLunInSuperPu] = TRUE;
            ucInvalidTargtCnt++;
        }
        else
        {
            /* compare valid Lun VBN and TS */
            if (uTempVirNum == INVALID_4F)
            {
                uTempVirNum = ulLunBlk[ucLunInSuperPu];
            }
            else
            {
                uTempVirNum1 = ulLunBlk[ucLunInSuperPu];
            }

            if ((uTempVirNum != uTempVirNum1) && (uTempVirNum != INVALID_4F) && (uTempVirNum1 != INVALID_4F))
            {
                /* This case is that PPO distance fo each Lun is over 1 and cross 2 target block.
                             Before abnormal shutdown, FTL send write super page RPMT to closed current target block. 
                             However, some Lun doesn't finish write RPMT, and other Lun start to write new target block from page 0. 
                             Next SPOR boot, FullRecovery find 2 VBN exist in one target block. for example: 

                             Lun0 VB:457 PB:17 pg:511 W SLC
                             Lun2 VB:457 PB:17 pg:511 W SLC
                             Lun1 VB:457 PB:17 pg:511 W SLC
                             Lun1 VB:456 PB:18 pg:0 W SLC 
                             Lun1 VB:456 PB:18 pg:1 W SLC
                             Lun1 VB:456 PB:18 pg:2 W SLC
                             
                             We can see that Lun 0 and 2 don't  trigger VB 456 page 0 ~ 2 write command after sending VB 457 RPMT write.
                              During FullRecovery, FW find Lun 0 target block is VB 457, Lun 1 target block is VB 456, Lun 2 has no target block.
                              FW should use two target block (457/456) PPO to check whose PPO is larger. We give up VB 456 data, let VB 457 as target block
                */
                DBG_Printf("[%s]SPU %d Lun%d TargetType %d different (PrevVB %d-CurVB %d)\n",
                    __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, ucTargetType, uTempVirNum, uTempVirNum1);
				bCompareErr = TRUE;
                //DBG_Getch();
            }
        }
    }

    if (bCompareErr == TRUE)
    {
        /* debug use, need delete later */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            DBG_Printf("Lun %d VBN %d\n", ucLunInSuperPu, ulLunBlk[ucLunInSuperPu]);
        }
		/* PPO distance is over 1 and cross 2 same target type target block */
        DBG_Printf("[%s]SPU%d target type %d has the different VBN, need to handle\n", __FUNCTION__, ucSuperPuNum, ucTargetType);
        L2_TB_Rebuild_FindCorrectTargetBlk(ucSuperPuNum, ucTargetType);
        return FALSE;
    }

#ifdef SIM
    if (ucInvalidTargtCnt == LUN_NUM_PER_SUPERPU)
    {
        /* Debug message : all LUN don't have target block, must not enter this function */
        DBG_Printf("[%s]SPU%d blktype%d no target block error\n", __FUNCTION__, ucSuperPuNum, ucTargetType);
        DBG_Getch();
    }
#endif

    if ((ucInvalidTargtCnt > 0) && (ucInvalidTargtCnt != LUN_NUM_PER_SUPERPU))
    {
#ifdef SIM
        if (uTempVirNum == INVALID_4F)
        {
            DBG_Printf("[%s]SPU %d Targettype %d: target block VBN %d error\n", __FUNCTION__, ucSuperPuNum, ucTargetType, uTempVirNum);
			DBG_Getch();
        }
#endif

        /* some target block Luns have no mapping PBN, need check which no mapping case is:
             case 1: some Lun only write first page, some Lun are empty: need allocate free block to empty Lun
             case 2: some Lun are closed blocks, some Lun are not closed (RPMT is still not written): need to use VBT to 
                        recovery target block information */
        if (L2_TB_Rebuild_FindTargetBlkMapping(ucSuperPuNum, uTempVirNum, ucTargetType) != TRUE)
        {
            /* case 1: allocate new free block and set target blk and PPO information */
            /* get current block type. If some lun of target block only write page 0, some isn't written and block type is default value,
			    rebuild PuInfo check each Lun block type of target block will getch */
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (ulLunBlk[ucLunInSuperPu] != INVALID_4F)
                {
                    uPhyBlk = pVBT[ucSuperPuNum]->m_VBT[ulLunBlk[ucLunInSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
                    ucCurBlockType = pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][uPhyBlk].BlockType;
					break;
                }
            }

            /* debug use */
#ifdef SIM
            if (ucCurBlockType == INVALID_2F)
            {
                DBG_Printf("[%s]blk type error\n", __FUNCTION__);
                DBG_Getch();
            }
#endif
            L2_TB_Rebuild_AllocateFreeBlkForLun(ucSuperPuNum, uTempVirNum, ucTargetType, ucCurBlockType);
        }
    }
    else
    {
        if (uTempVirNum < TLC_BLK_CNT)
        {
            DBG_Printf("[%s]SPU%d VBN%d error, TLC has target blk after rebuild\n", __FUNCTION__, ucSuperPuNum, uTempVirNum);
            DBG_Getch();
        }

        if (ucInvalidTargtCnt == LUN_NUM_PER_SUPERPU)
        {
            DBG_Printf("[%s]SPU%d targettype %d all Lun no mapping error\n", __FUNCTION__, ucSuperPuNum, ucTargetType);
            DBG_Getch();
        }
    }

    return TRUE;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_Proc2TargetBlkExistCase(U8 ucSuperPuNum, BOOL* bIsNeedEraseTargetBlk)
{
    U8 ucTargetIndex, ucLunInSuperPu;
    U8 ucTargetBlkCnt;
    U16 uFirstVBN, uSecondVBN;
    U16 uFirstPBN, uSecondPBN;
    U32 ulFirstBlkTS, ulSecondBlkTS;
    TargetType ucTargetType;

    for (ucTargetType = 0; ucTargetType < (TARGET_ALL - 1); ucTargetType++)
    {
        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
       	    if (pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType] != TEMP_BLK_CNT_PER_TARGET)
                continue;

            ulFirstBlkTS = INVALID_8F;
            ulSecondBlkTS = INVALID_8F;
            uFirstVBN = INVALID_4F;
            uSecondVBN = INVALID_4F;
            uFirstPBN = INVALID_4F; 
            uSecondPBN = INVALID_4F;

            ucTargetBlkCnt = 0;
            for (ucTargetIndex = 0; ucTargetIndex < TEMP_BLK_CNT_PER_TARGET; ucTargetIndex++)
            {
                if (INVALID_4F != pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
                {
                    if (ucTargetIndex == 0)
                    {
                        uFirstVBN = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
                        uFirstPBN = pVBT[ucSuperPuNum]->m_VBT[uFirstVBN].PhysicalBlockAddr[ucLunInSuperPu];
                        ulFirstBlkTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][uFirstPBN].LastPageTimeStamp;
                    }
                    else
                    {
                        uSecondVBN = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex];
                        uSecondPBN = pVBT[ucSuperPuNum]->m_VBT[uSecondVBN].PhysicalBlockAddr[ucLunInSuperPu];
                        ulSecondBlkTS = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][uSecondPBN].LastPageTimeStamp;
                    }

                    ucTargetBlkCnt++;
					/* Lun has two open block, need double check */
                    if (ucTargetBlkCnt == TEMP_BLK_CNT_PER_TARGET)
                    {
                        DBG_Printf("[%s]SPU %d Lun%d target block type %d has two target block\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu, ucTargetType);
                        /* use latest page TS of PBIT to compare which block is the correct target block */
                        if (uFirstVBN == uSecondVBN)
                        {
                            DBG_Printf("[%s]there has 2 same VB%d_%d target block\n",
                                __FUNCTION__, uFirstVBN, uSecondVBN);
                            DBG_Getch();
                        }

                        if (ulFirstBlkTS > ulSecondBlkTS)
                        {
                            DBG_Printf("[%s]Lun %d 1stVBN 0x%x (PBN:0x%x) TS 0x%x > 2ndVBN %d (PBN:%d) TS 0x%x, 2ndPBN need erase\n",
                                __FUNCTION__, ucLunInSuperPu, uFirstVBN, uFirstPBN, ulFirstBlkTS, uSecondVBN, uSecondPBN, ulSecondBlkTS);
                            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = uSecondPBN;
                            pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][1] = INVALID_4F;
                        }
                        else
                        {
                            DBG_Printf("[%s]Lun %d 2ndVBN 0x%x (PBN:0x%x) TS 0x%x > 1stVBN %d (PBN:%d) TS 0x%x, 1stPBN need erase\n",
                                __FUNCTION__, ucLunInSuperPu, uSecondVBN, uSecondPBN, ulSecondBlkTS, uFirstVBN, uFirstPBN, ulFirstBlkTS);
                            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = uFirstPBN;						
                            pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][0] = INVALID_4F;
                        }

                        *bIsNeedEraseTargetBlk = TRUE;
                    }
                }
            }
        }

        if (*bIsNeedEraseTargetBlk == TRUE)
        {
            return;
        }
    }

	*bIsNeedEraseTargetBlk = FALSE;
}

BOOL MCU1_DRAM_TEXT L2_TB_Rebuild_HasTargetBlk(U8 ucSuperPuNum, TargetType ucTargetType)
{
    U8 ucTargetIndex, ucLunInSuperPu;
    BOOL bHasTarget = FALSE;

    /* first check and set each LUN target index. If one Lun has valid block, it means current target type of SuperPU has the valid target block.
          We also need to set each valid target block of Lun.
          For example 1: if super pu 0 Lun 0 has the valid target block and its TargetIndex is 1, it means the taget block VBN is at TargetIndex 0.
          Therefore, set TargetIndex from 1 to 0
          For example 2: if super pu 0 Lun 0 hasn't the invalid target block, it means its TargetIndex is at 0 and corresponding TargetBlock is INVALID_4F */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (ucTargetIndex = 0; ucTargetIndex < TEMP_BLK_CNT_PER_TARGET; ucTargetIndex++)
        {
            if (INVALID_4F != pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex])
            {
                /* set final target block index */
                pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType] = ucTargetIndex;

                if (bHasTarget == FALSE)
                {
                    bHasTarget = TRUE;
                }
            }
        }
    }

    return bHasTarget;
}

void MCU1_DRAM_TEXT L2_TB_Rebuild_FillRebuildRPMTInfo(U8 ucSuperPuNum, U8 ucLunInSuperPu, U8 CurRPMTTargetType)
{
    U8 ucTargetIndex;
    U16 usVirBlk, usPhyBlk;

    ucTargetIndex = pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][CurRPMTTargetType];
    usVirBlk = pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][CurRPMTTargetType][ucTargetIndex];
    usPhyBlk = pVBT[ucSuperPuNum]->m_VBT[usVirBlk].PhysicalBlockAddr[ucLunInSuperPu];
    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] = 0;
    pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = usPhyBlk;
    DBG_Printf("SPU%d Lun%d Blk%d_%d need rebuild RPMT\n", ucSuperPuNum, ucLunInSuperPu, usVirBlk, usPhyBlk);
}


U8 MCU1_DRAM_TEXT L2_TB_Rebuild_CheckScanResult(U8 ucSuperPuNum)
{
    U8 ucRet, ucLunInSuperPu, ucSaveBBTDoneCnt;
    U8 ucMappingCnt, ucFlashStatus, ucIsTLCBlk;
    U16 usVirBlk, usPhyBlk, i;
#ifdef READ_DISTURB_OPEN
    U16 usPBN;
#endif
    BOOL bHasTargetBlk, bNeedErase;
    static BOOL bIsNeedEraseTargetBlk[SUBSYSTEM_SUPERPU_MAX];
    static BOOL bIsNeedRebRPMT[SUBSYSTEM_SUPERPU_MAX];
    static BOOL bNeedEraseAfterResume[SUBSYSTEM_SUPERPU_MAX];
    static BOOL bStartEraseInforHandling[SUBSYSTEM_SUPERPU_MAX];
    L2_TB_REBUILD_CHECK_STATUS *pStatus;
    TargetType ucTargetType; 
    static TargetType CurRPMTTargetType[SUBSYSTEM_SUPERPU_MAX];
    BlkType ucBlkType;
    PhysicalAddr Addr;
    RED* pRed;
    RED DummyWrRed;

    ucRet = L2_TB_WAIT;
    pStatus = &(pTBRebManagement->CheckScanResultStatus[ucSuperPuNum]);

    switch (*pStatus)
    {
    case L2_TB_REBUILD_CHECK_START:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            bNeedSaveBBT[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            ulLunBlk[ucLunInSuperPu] = INVALID_4F;
            ulLunBlkTS[ucLunInSuperPu] = INVALID_8F;
        }

        /* use LUN 0 index as the virtual block index */
        pTBRebManagement->usBlockIndex[ucSuperPuNum][0] = 0;
        bSaveBBT[ucSuperPuNum] = FALSE;
        SLCIndex[ucSuperPuNum] = 0;		
        bIsNeedRebRPMT[ucSuperPuNum] = FALSE;
        bNeedEraseAfterResume[ucSuperPuNum] = FALSE;
        bStartEraseInforHandling[ucSuperPuNum] = FALSE;
        CurRPMTTargetType[ucSuperPuNum] = TARGET_HOST_WRITE_NORAML;
        *pStatus = L2_TB_REBUILD_CHECK_PROCESS_TARGET_BLOCK;
    }
        //break;

    case L2_TB_REBUILD_CHECK_PROCESS_TARGET_BLOCK:
    {
        L2_TB_Rebuild_Proc2TargetBlkExistCase(ucSuperPuNum, &bIsNeedEraseTargetBlk[ucSuperPuNum]);
		
        if (bIsNeedEraseTargetBlk[ucSuperPuNum] == TRUE)
        {
            *pStatus = L2_TB_REBUILD_CHECK_ERASE_BLOCK;
            break;
        }

        for (ucTargetType = 0; ucTargetType < (TARGET_ALL - 1); ucTargetType++)
        {
            bNeedLoadPRMT[ucTargetType] = FALSE;
            bHasTargetBlk = L2_TB_Rebuild_HasTargetBlk(ucSuperPuNum, ucTargetType);
            if (bHasTargetBlk == TRUE)
            {
                if(!L2_TB_Rebuild_CheckTargetBlock(ucSuperPuNum, ucTargetType))
                {
                    bIsNeedEraseTargetBlk[ucSuperPuNum] = TRUE;
                    *pStatus = L2_TB_REBUILD_CHECK_ERASE_BLOCK;
                    break;
                }
            }
        }
        *pStatus = L2_TB_REBUILD_CHECK_VBN_PBN_MAPPING;
    }
        break;

    case L2_TB_REBUILD_CHECK_VBN_PBN_MAPPING: // need double check: target block may be check again?
    {
        /* only need to check TLC virtual block cnt(496) + SLC virtual block cnt (40), SLC table block (16) needn't to check */
        for (usVirBlk = pTBRebManagement->usBlockIndex[ucSuperPuNum][0]; usVirBlk < VIR_BLK_CNT; usVirBlk++)
        {
            ucMappingCnt = 0;

            /* bPhyAddFindInFullRecovery is bitmap, so only check bitmap isn't 0 virtual block.  !0:found;0:not found */
            if (pVBT[ucSuperPuNum]->m_VBT[usVirBlk].bPhyAddFindInFullRecovery == FALSE)
                continue;

            if (L2_TB_Rebuild_IsCurVirBlkTarget(ucSuperPuNum, usVirBlk) == TRUE)// need modify
            {
                L2_PBIT_Increase_AllocBlockCnt(ucSuperPuNum, BLKTYPE_SLC);
                continue;
            }

            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                ulLunBlkTS[ucLunInSuperPu] = 0;
                ulLunBlk[ucLunInSuperPu] = INVALID_4F;

                if (pVBT[ucSuperPuNum]->m_VBT[usVirBlk].PhysicalBlockAddr[ucLunInSuperPu] != 0)
                {
                    usPhyBlk = pVBT[ucSuperPuNum]->m_VBT[usVirBlk].PhysicalBlockAddr[ucLunInSuperPu];
                    ucIsTLCBlk = L2_TB_Is_TLCBlk(ucSuperPuNum, ucLunInSuperPu, usPhyBlk);
                    /* SLC super block: get first page TS, SLC super block: get last page TS*/
                    if (ucIsTLCBlk == TRUE)
                    {
                        //TLC block last page UECC TS = 0 case need to check
                        ulLunBlkTS[ucLunInSuperPu] = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usPhyBlk].LastPageTimeStamp;
                    }
                    else
                    {
                        ulLunBlkTS[ucLunInSuperPu] = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usPhyBlk].TimeStamp;
                    }

                    /* record Lun mapping PBN */
                    ulLunBlk[ucLunInSuperPu] = usPhyBlk;
                    ucMappingCnt++;
                }
            }

            if (ucMappingCnt == LUN_NUM_PER_SUPERPU)
            {
                /* now all Lun of current virtual super block has all phyical block mapping, increase TotalAllocateBlkCnt */
                if (ucIsTLCBlk == TRUE)
                {
                    ucBlkType = BLKTYPE_TLC;

#ifdef READ_DISTURB_OPEN

                    /* Reset ReadCnt for TLC allocated block when rebuild VBN/PBN mapping done */
                    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                    {
                        usPBN = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlk);
                        L2_ResetBlkReadCnt(ucSuperPuNum, ucLunInSuperPu, usPBN);
                    }
#endif
                }
                else
                {
                    ucBlkType = BLKTYPE_SLC;
                }

                L2_PBIT_Increase_AllocBlockCnt(ucSuperPuNum, ucBlkType);
            }
            else
            {
                bNeedErase = FALSE;

                if (usVirBlk < TLC_BLK_CNT)
                {
                    DBG_Printf("[%s]TLC VBN %d has no full PBN mapping\n", __FUNCTION__, usVirBlk);
                    if (TRUE != L2_TB_RebuildFindTSinErrTLCBlk(ucSuperPuNum, usVirBlk))
                    {
                        DBG_Printf("[%s]SPU%d TLC VBN%d cannot find the same TS in the ErrTLCblock, erase\n", __FUNCTION__, 
                            ucSuperPuNum, usVirBlk);
                        bNeedErase = TRUE;
                    }
                }
                else
                {
                    DBG_Printf("[%s]SPU%d SLC VBN%d cannot have full PBN mapping, erase\n", __FUNCTION__, 
                            ucSuperPuNum, usVirBlk);
                    bNeedErase = TRUE;
                }

				if (bNeedErase == TRUE)
                {
                    /* the virtual block don't have all LUN physical mapping, record Lun physical block number to erase */
                    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                    {
                        pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = ulLunBlk[ucLunInSuperPu];
                        DBG_Printf("[%s]SPU%d VBN%d Lun%d phyblk%d\n", __FUNCTION__, ucSuperPuNum,usVirBlk, ucLunInSuperPu, ulLunBlk[ucLunInSuperPu]);

                        if (ulLunBlk[ucLunInSuperPu] != INVALID_4F)
                        {
                            pVBT[ucSuperPuNum]->m_VBT[usVirBlk].bPhyAddFindInFullRecovery &= ~(1 << ucLunInSuperPu);
                        }
                    }

                    /* record current scan virtual block index */
                    pTBRebManagement->usBlockIndex[ucSuperPuNum][0] = usVirBlk + 1;
                    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
                    break;
                }

                /* now all missing Luns have mapping, increase TotalAllocateBlkCnt */
                if (ucIsTLCBlk == TRUE)
                {
                    ucBlkType = BLKTYPE_TLC;

#ifdef READ_DISTURB_OPEN
                    /* Reset ReadCnt for TLC allocated block when rebuild VBN/PBN mapping done */
                    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                    {
                        usPBN = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlk);
                        L2_ResetBlkReadCnt(ucSuperPuNum, ucLunInSuperPu, usPBN);
                    }
#endif
                }
                else
                {
                    ucBlkType = BLKTYPE_SLC;
                }

                L2_PBIT_Increase_AllocBlockCnt(ucSuperPuNum, ucBlkType);
            }
        }
        *pStatus = L2_TB_Rebuild_CheckMappingNextStage(ucSuperPuNum, usVirBlk);
    }
        break;

    case L2_TB_REBUILD_CHECK_ERASE_BLOCK:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] == INVALID_4F)
            {
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                continue;
            }

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                if (INVALID_4F == pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu])
                {
                    /* debug use */
                    DBG_Printf("[%s]SPU%d LUN%d ERASE Block ERROR!\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu);
                    DBG_Getch();
                }
                else
                {
                    /* current LUN can send command, mark bitmap corresponding LUN bit */
                    pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                    if (TRUE == L2_TB_Is_TLCBlk(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu]))
                    {
                        L2_EraseTLCBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu],
							&(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), FALSE);
                    }
                    else
                    {
                        L2_FtlEraseBlock(ucSuperPuNum, ucLunInSuperPu, pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu],
							&(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, FALSE);
                    }
                }
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_REBUILD_CHECK_WAIT_ERASE_BLOCK;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_WAIT_ERASE_BLOCK:
    {
        /* wait for flash idle */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] != INVALID_4F)
            {
                if (SUBSYSTEM_STATUS_PENDING == pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu])
                {
                    return ucRet;
                }
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] == INVALID_4F)
                continue;

            usPhyBlk = pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu];
			usVirBlk = pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlk].VirtualBlockAddr;
            ucFlashStatus = pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
            {
                /* case 1: 2 data target block, 
                   case 2: VBN X doesn't has full PBN mapping, after erase, need to reset PBIT and VBT mapping to invalid*/
                L2_TB_Rebuild_DataBlockSetFree(ucSuperPuNum, ucLunInSuperPu, usPhyBlk);
                L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usPhyBlk);
                L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usPhyBlk);

                if (bNeedEraseAfterResume[ucSuperPuNum] == FALSE && bStartEraseInforHandling[ucSuperPuNum] == FALSE)
                {
                    L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usPhyBlk, INVALID_4F);
                    L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlk, 0);
#ifdef SPOR_DEBUGLOG_ENABLE
				    DBG_Printf("E VB %d PB %d done\n", usVirBlk, usPhyBlk);
#endif
                }
            }
            else
            {
                /* erase fail - save BBT and and set current block error */
                L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usPhyBlk, FALSE);
                L2_PBIT_Set_Allocate(ucSuperPuNum, ucLunInSuperPu, usPhyBlk, FALSE);
                L2_PBIT_Set_Error(ucSuperPuNum, ucLunInSuperPu, usPhyBlk, TRUE);
                L2_Reset_PBIT_Info(ucSuperPuNum, ucLunInSuperPu, usPhyBlk);
                L2_PBIT_Increase_EraseCnt(ucSuperPuNum, ucLunInSuperPu, usPhyBlk);
                L2_BbtAddBbtBadBlk(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), usPhyBlk, RT_BAD_BLK, ERASE_ERR);
#ifdef SPOR_DEBUGLOG_ENABLE
				DBG_Printf("E VB %d PB %d fail\n", usVirBlk, usPhyBlk);
#endif
                if (bNeedSaveBBT[ucSuperPuNum][ucLunInSuperPu] == FALSE)
                {
                    bNeedSaveBBT[ucSuperPuNum][ucLunInSuperPu] = TRUE;
                }

                if (bSaveBBT[ucSuperPuNum] == FALSE)
                {
                    bSaveBBT[ucSuperPuNum] = TRUE;
                }

                gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
            }

			/* after TLC block is erased, we need to check whether current TLC is in TLC last page UECC array.
			If yes, reset it.*/
            if (usVirBlk < TLC_BLK_CNT)
            {
                for (i = 0; i < pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu]; i++)
                {
                    if (usPhyBlk == pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i])
                    {
                        /*Remove Block from ErrTLCBlk Que*/
                        pTBRebManagement->usErrTLCBlk[ucSuperPuNum][ucLunInSuperPu][i] = INVALID_4F;
#ifdef SPOR_DEBUGLOG_ENABLE
                        DBG_Printf("[%s]Lun %d Remove ErrTLCBlk %d from queue after erasing\n", __FUNCTION__, ucLunInSuperPu, usPhyBlk);
#endif
                    }
                }
            }

#ifdef SIM
            if (bNeedEraseAfterResume[ucSuperPuNum] == TRUE)
                DBG_Printf("resume target block: PBN %d done\n", usPhyBlk);
            else if (bStartEraseInforHandling[ucSuperPuNum] == TRUE)
                DBG_Printf("Erase shutdown handling: PBN %d done\n", usPhyBlk);
#endif

            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            if (bNeedEraseAfterResume[ucSuperPuNum] == FALSE && bStartEraseInforHandling[ucSuperPuNum] == FALSE)
                pVBT[ucSuperPuNum]->m_VBT[usVirBlk].bPhyAddFindInFullRecovery &= ~(1 << ucLunInSuperPu);
        }

        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
        if (bIsNeedEraseTargetBlk[ucSuperPuNum] == TRUE)
        {
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("Erase 1 error target VBN %d done\n", usVirBlk);
#endif
            *pStatus = L2_TB_REBUILD_CHECK_PROCESS_TARGET_BLOCK;
            bIsNeedEraseTargetBlk[ucSuperPuNum] = FALSE;
        }
        else
        {
#ifdef SPOR_DEBUGLOG_ENABLE
            if (bNeedEraseAfterResume[ucSuperPuNum] == FALSE && bStartEraseInforHandling[ucSuperPuNum] == FALSE)
                DBG_Printf("Erase 1 no full mapping VBN %d done\n", usVirBlk);
            else if (bStartEraseInforHandling[ucSuperPuNum] == TRUE)
                DBG_Printf("Erase shutdown handling done\n");
            else
                DBG_Printf("Erase targetblk %d\n", usVirBlk);
#endif
            *pStatus = L2_TB_Rebuild_CheckEraseNextStage(ucSuperPuNum, bNeedEraseAfterResume[ucSuperPuNum], bStartEraseInforHandling[ucSuperPuNum]);
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_ERASE_SAVE_BBT:
    {
        /* save BBT */
        ucSaveBBTDoneCnt = 0;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (bNeedSaveBBT[ucSuperPuNum][ucLunInSuperPu] == FALSE)
            {
                ucSaveBBTDoneCnt++;
                continue;
            }
            if (TRUE == L2_BbtSave(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), INVALID_2F))
            {
                bNeedSaveBBT[ucSuperPuNum][ucLunInSuperPu] = FALSE;
                gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
                ucSaveBBTDoneCnt++;
            }
        }

        if (ucSaveBBTDoneCnt == LUN_NUM_PER_SUPERPU)
        {
            if (bHandleSPORDuringErase[ucSuperPuNum] == FALSE)
                *pStatus = L2_TB_REBUILD_CHECK_SUCCESS;
            else
            {
                HAL_DMAESetValue(g_L2TempBufferAddr, BUF_SIZE, 0);
                *pStatus = L2_TB_REBUILD_CHECK_DUMMYWRITE_AT1;
            }
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_FINISH:
    {
        /* allocate free virtual block */
        L2_TB_Rebuild_DataAllocateFreeBlock(ucSuperPuNum);

        bNeedEraseAfterResume[ucSuperPuNum] = L2_TB_Rebuild_DataResumeTargetBlockInfo(ucSuperPuNum);
        if (bNeedEraseAfterResume[ucSuperPuNum] == FALSE)
        {
            if (bHandleSPORDuringErase[ucSuperPuNum] == TRUE)
                *pStatus = L2_TB_REBUILD_CHECK_HANLDE_ERASE_INFO;
            else
                *pStatus = L2_TB_Rebuild_IsNeedSaveBBT(ucSuperPuNum);
        }
        else
        {
            *pStatus = L2_TB_REBUILD_CHECK_ERASE_BLOCK;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_LOAD_TARGET_RPMT:
    {
        for (ucTargetType = CurRPMTTargetType[ucSuperPuNum]; ucTargetType < (TARGET_ALL - 1); ucTargetType++)
        {
            if(L2_TB_Rebuild_IsNeedLoadClosedTargetBlkRPMT(ucSuperPuNum, ucTargetType))
            {
                if (bNeedLoadPRMT[ucTargetType] == TRUE)
                {
                    CurRPMTTargetType[ucSuperPuNum] = ucTargetType;
                    *pStatus = L2_TB_REBUILD_CHECK_WAIT_LOAD_TARGET_RPMT;
                    bIsNeedRebRPMT[ucSuperPuNum] = FALSE;
                    break;
                }
            }
            else
            {
                if (ucTargetType == TARGET_HOST_GC)
                    *pStatus = L2_TB_REBUILD_CHECK_FINISH;
            }
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_WAIT_LOAD_TARGET_RPMT:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (TRUE == COM_BitMaskGet(pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum], ucLunInSuperPu))
                continue;

            if (FALSE == COM_BitMaskGet(g_PuInfo[ucSuperPuNum]->m_RPMTFlushBitMap[CurRPMTTargetType[ucSuperPuNum]], ucLunInSuperPu))
            {
                pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
                continue;
            }

            if (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_PENDING)
            {
                continue;
            }
            else if ((pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_SUCCESS) ||
                (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_RETRY_SUCCESS) ||
                (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_RECC))
            {
                pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            }
            else if (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_FAIL)
            {
                /* some Lun RPMT read UECC, need Rebuild, record this Lun's target physical block. */
				L2_TB_Rebuild_FillRebuildRPMTInfo(ucSuperPuNum, ucLunInSuperPu, CurRPMTTargetType[ucSuperPuNum]);
                bIsNeedRebRPMT[ucSuperPuNum] = TRUE;
            }
            else
            {
                /* RPMT is empty case: abnormal shutdown during program RPMT, first read is UECC (N1 9328), second read is empty page (N1 > 9340)
                             don't getch, try to rebuild RPMT */
                DBG_Printf("[%s]SPU%d Lun%d load RPMT empty error\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu,
                     g_PuInfo[ucSuperPuNum]->m_RPMTFlushBitMap[CurRPMTTargetType[ucSuperPuNum]]);
                L2_TB_Rebuild_FillRebuildRPMTInfo(ucSuperPuNum, ucLunInSuperPu, CurRPMTTargetType[ucSuperPuNum]);
                bIsNeedRebRPMT[ucSuperPuNum] = TRUE;
                //DBG_Getch();
            }
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            if (bIsNeedRebRPMT[ucSuperPuNum] == FALSE)
            {
                /* current target block load RPMT done, check whether all target is check done */
                if (CurRPMTTargetType[ucSuperPuNum] == TARGET_HOST_GC)
                {
                    *pStatus = L2_TB_REBUILD_CHECK_FINISH;
                }
                else
                {
                    CurRPMTTargetType[ucSuperPuNum]++;
                    *pStatus = L2_TB_REBUILD_CHECK_LOAD_TARGET_RPMT;
                }
            }
            else
                *pStatus = L2_TB_REBUILD_CHECK_READ_NEXT_PAGE;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_READ_NEXT_PAGE:
    {
        /* use super page to rebuild RPMT if Lun needs */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] == INVALID_4F)
            {
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
                continue;
            }

            /* read next page redundant of each block */
            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                /* get flashaddr, temp red and send read request to L3 */
                Addr.m_PUSer = ucSuperPuNum;
                Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                Addr.m_BlockInPU = pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];
                Addr.m_PageInBlock = pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu];
                Addr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_LoadSpare(&Addr, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                    (U32 *)pRed, TRUE);

                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            *pStatus = L2_TB_REBUILD_CHECK_WAIT_READ_NEXT_PAGE;
        }
    }
        break;
    case L2_TB_REBUILD_CHECK_WAIT_READ_NEXT_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] == INVALID_4F)
            {
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
                continue;
            }

            if (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_PENDING)
            {
                continue;
            }
            else if ((pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_SUCCESS) ||
                (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_RETRY_SUCCESS) ||
                (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_RECC))
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_TB_Rebuild_DataSetRPMT(ucSuperPuNum, ucLunInSuperPu, pRed);
            }
            else if (pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] == SUBSYSTEM_STATUS_FAIL)
            {
                /* fill invalid RED to RPMT*/
                L2_TB_Rebuild_DataSetInvalidRPMT(ucSuperPuNum, ucLunInSuperPu, CurRPMTTargetType[ucSuperPuNum]);
            }
            else
            { 
                /* rebuild RPMT finds data page is empty error */
                DBG_Printf("[%s]SPU%d Lun%d PB %d page is empty error\n", __FUNCTION__, ucSuperPuNum, ucLunInSuperPu,
                    pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu], 
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]);
                DBG_Getch();
            }

            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (1 << ucLunInSuperPu);
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] != INVALID_4F)
                {
                    pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]++;
                    if (pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu] == (PG_PER_SLC_BLK - 1))
                    {
                        /* update RPMT superPU and superblk information */
                        L2_TB_Rebuild_DataSetOtherRPMTInfo(ucSuperPuNum, ucLunInSuperPu, CurRPMTTargetType[ucSuperPuNum]);
                        *pStatus = L2_TB_REBUILD_CHECK_FINISH;
                    }
                }
            }
			
            if (*pStatus != L2_TB_REBUILD_CHECK_FINISH)
            {
                *pStatus = L2_TB_REBUILD_CHECK_READ_NEXT_PAGE;
            }
            else
            {
                /* check whether all target is check done */
                if (CurRPMTTargetType[ucSuperPuNum] == TARGET_HOST_GC)
                {
                    *pStatus =L2_TB_REBUILD_CHECK_FINISH;
                }
                else
                {
                    CurRPMTTargetType[ucSuperPuNum]++;
                    *pStatus = L2_TB_REBUILD_CHECK_LOAD_TARGET_RPMT;
                }
            }
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_HANLDE_ERASE_INFO:
    {
        if (L2_TB_Rebuild_HandleEraseInfo(ucSuperPuNum))
        {
            bStartEraseInforHandling[ucSuperPuNum] = TRUE;
            pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            *pStatus = L2_TB_REBUILD_CHECK_ERASE_BLOCK;
        }
        else
        {
            if (bHandleSPORDuringErase[ucSuperPuNum] == FALSE)
                *pStatus = L2_TB_REBUILD_CHECK_SUCCESS;
            else
            {
                HAL_DMAESetValue(g_L2TempBufferAddr, BUF_SIZE, 0);
                *pStatus = L2_TB_REBUILD_CHECK_DUMMYWRITE_AT1;
            }
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_DUMMYWRITE_AT1:
    {
        if (FALSE == L2_TB_Rebuild_CheckAT1TargetBlk(ucSuperPuNum))
        {
            *pStatus = L2_TB_REBUILD_CHECK_SUCCESS;
            break;
        }

        pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                Addr.m_PUSer = ucSuperPuNum;
                Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                Addr.m_BlockInPU = pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu];
                Addr.m_PageInBlock = pRT->m_RT[ucSuperPuNum].CurAT1PPO;
                Addr.m_LPNInPage = 0;

                DummyWrRed.m_RedComm.eOPType = OP_TYPE_DUMMY_WRITE;
                DummyWrRed.m_RedComm.bcPageType = PAGE_TYPE_DUMMY;
                DummyWrRed.m_RedComm.bcBlockType = BLOCK_TYPE_PMT;

                DBG_Printf("Lun%d dummy wr AT1 %d pg %d\n", ucLunInSuperPu, Addr.m_BlockInPU, Addr.m_PageInBlock);
                L2_FtlWriteLocal(&Addr, (U32 *)g_L2TempBufferAddr, (U32*)&DummyWrRed, &(pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, NULL);
            }
        }
            
        if (SUPERPU_LUN_NUM_BITMSK == (pTBRebManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            pRT->m_RT[ucSuperPuNum].CurAT1PPO++;
            *pStatus = L2_TB_REBUILD_CHECK_WAIT_DUMMYWRITE_AT1;
        }
    }
        break;

    case L2_TB_REBUILD_CHECK_WAIT_DUMMYWRITE_AT1:
    {
        /* wait for flash idle */
        if (FALSE== L2_TB_Rebuild_WaitAllFlashFinish(ucSuperPuNum, (L2_TB_REBUILD_STATUS *) pStatus))
        {
            return ucRet;
        }

        if (pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] == LUN_NUM_PER_SUPERPU)
        {
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("SPU%d dummy wr AT1 page %d success\n", ucSuperPuNum, pRT->m_RT[ucSuperPuNum].CurAT1PPO);
#endif
        }
        else
        {
            /* some Lun program fail, do nothing. When fw read this program fail page next time, it will be UECC. This page is 
                       dummy data, so read UECC can be accepted. */
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("SPU%d dummy wr AT1 page %d fail\n", ucSuperPuNum, pRT->m_RT[ucSuperPuNum].CurAT1PPO);
#endif
        }

        *pStatus = L2_TB_REBUILD_CHECK_SUCCESS;
    }
        break;

    case L2_TB_REBUILD_CHECK_SUCCESS:
    {
        ucRet = L2_TB_SUCCESS;
    }
        break;
    case L2_TB_REBUILD_CHECK_FAIL:
    {
        ucRet = L2_TB_FAIL;
    }
        break;
    default:
    {
        DBG_Printf("[%s] invalid Status 0x%x\n", __FUNCTION__, *pStatus);
        DBG_Getch();
    }
        break;

    }

    return ucRet;
}

BOOL MCU1_DRAM_TEXT L2_TB_RT_IsNeedRebuild(void)
{
    BOOL bRTNeedRebuild = FALSE;
	U8  ucSuperPuNum;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        if (bRTHasLoad[ucSuperPuNum] == FALSE)
        {
            bRTNeedRebuild = TRUE;
			break;
        }
    }

    return bRTNeedRebuild;
}

U32 MCU1_DRAM_TEXT L2_TB_FullRecovery(void)
{
    U8  ucSuperPuNum, ucLunInSuperPu;;
    U8  ucCheckPuCnt;
    U32 ulRet;
    U32 ulCmdRet[SUBSYSTEM_SUPERPU_MAX];
    U32 ulScanDataBlkCmdRet[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    L2_TB_FULLRECOVERY_STATUS *pStatus;

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBRebManagement->FullRecoveryStatus);

    switch (*pStatus)
    {
    case L2_TB_FULLRECOVERY_START:
    {
        /* Init global values */
        if (L2_TB_RT_IsNeedRebuild() == TRUE)
        {
            /* RT need rebuild, reset all RT data structure*/
            L2_RT_Init();
        }
        else
        {
            /* RT load success, only reset AT0/AT1/Trace block corresponding infor in RT data structure*/
            L2_RT_Init_TableLocation();
        }

        L2_PBIT_Init(FALSE, TRUE);
        L2_VBT_Init();

        L2_TB_ResetRebuildStatus();
        L2_TB_ResetEraseAllStatus();

        bTBRebuildPending = TRUE;
        *pStatus = L2_TB_FULLRECOVERY_REBUILD_RT;
    }
        //break;

    case L2_TB_FULLRECOVERY_REBUILD_RT:
    {
        ucCheckPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_Rebuild_RT(ucSuperPuNum);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckPuCnt++;
            }
        }

        if (ucCheckPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_FULLRECOVERY_FAIL;
                    break;
                }
            }

            if (L2_TB_FULLRECOVERY_FAIL != *pStatus)
            {
                /* clear status and start rebuild AT0 AT1 block */
                L2_TB_ResetRebuildStatus();
                *pStatus = L2_TB_FULLRECOVERY_REBUILD_TABLE;
            }
        }
    }
        break;

    case L2_TB_FULLRECOVERY_REBUILD_TABLE:
    {
        ucCheckPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_Rebuild_TableBlock(ucSuperPuNum);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckPuCnt++;
            }
        }

        if (ucCheckPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_FULLRECOVERY_FAIL;
                    break;
                }
            }

            if (L2_TB_FULLRECOVERY_FAIL != *pStatus)
            {
                /* clear status and start rebuild data block */
                L2_TB_ResetRebuildStatus();
                L2_TB_ResetEraseAllStatus();
                *pStatus = L2_TB_FULLRECOVERY_REBUILD_DATA;
            }
        }
    }
        break;

    case L2_TB_FULLRECOVERY_REBUILD_DATA:
    {
        ucCheckPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                ulScanDataBlkCmdRet[ucSuperPuNum][ucLunInSuperPu] = L2_TB_Rebuild_DataBlock(ucSuperPuNum, ucLunInSuperPu);

                if (L2_TB_WAIT != ulScanDataBlkCmdRet[ucSuperPuNum][ucLunInSuperPu])
                {
                    ucCheckPuCnt++;
                }
            }
        }

        if (ucCheckPuCnt >= (SUBSYSTEM_SUPERPU_NUM * LUN_NUM_PER_SUPERPU))
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    if (L2_TB_FAIL == ulScanDataBlkCmdRet[ucSuperPuNum][ucLunInSuperPu])
                    {
                        *pStatus = L2_TB_FULLRECOVERY_FAIL;
                        break;
                    }
                }
            }

            if (L2_TB_FULLRECOVERY_FAIL != *pStatus)
            {
                *pStatus = L2_TB_FULLRECOVERY_REBUILD_CHECK_SCAN_DATABLK;
            }
        }
    }
        break;

    case L2_TB_FULLRECOVERY_REBUILD_CHECK_SCAN_DATABLK:
    {
        ucCheckPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_Rebuild_CheckScanResult(ucSuperPuNum);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckPuCnt++;
            }
        }

        if (ucCheckPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_FULLRECOVERY_FAIL;
                    break;
                }
            }

            if (L2_TB_FULLRECOVERY_FAIL != *pStatus)
            {
                *pStatus = L2_TB_FULLRECOVERY_SUCCESS;
            }
        }
    }
        break;

    case L2_TB_FULLRECOVERY_FAIL:
    {
        L2_TB_ResetRebuildStatus();
        L2_TB_ResetFullRecoveryStatus();
        bTBRebuildPending = FALSE;
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_FULLRECOVERY_SUCCESS:
    {
        L2_TB_ResetRebuildStatus();
        L2_TB_ResetFullRecoveryStatus();
        bTBRebuildPending = FALSE;
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_FullRecovery invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

/********************** FILE END ***************/
