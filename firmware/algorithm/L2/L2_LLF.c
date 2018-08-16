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
Filename    :L2_LLF.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.2
Description :functions about LLF
1) L2_LLF(): LLF Functions
Others      :
Modify      :
****************************************************************************/
#include "HAL_Inc.h"
#include "L2_Defines.h"
#include "L2_Debug.h"
#include "L2_PMTManager.h"
#include "L2_StripeInfo.h"
#include "L2_GCManager.h"
#include "L2_RT.h"
#include "L2_LLF.h"
#include "L2_FTL.h"
#include "L2_TableBlock.h"
#include "COM_Memory.h"
#include "L2_TableBBT.h"
#include "L2_FCMDQ.h"

#include "COM_BitMask.h"
#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif
#include "L2_Erase.h"

LOCAL  U16 g_usInitPMTStatus;
LOCAL  U32 PMTPageCnt;
LOCAL  U16 FinishedPMTIIndex[SUBSYSTEM_SUPERPU_MAX];
LOCAL  U16 pRecordPPOs[SUBSYSTEM_SUPERPU_MAX];
LOCAL  U16 pRecordBlkSNs[SUBSYSTEM_SUPERPU_MAX];
LOCAL  U16 pRecordPMTIIndexs[SUBSYSTEM_SUPERPU_MAX];

GLOBAL  L2_LLF_STATUS gL2LLFStatus;
extern void MCU1_DRAM_TEXT L2_BM_LLF_Init_BlkSN(void);
extern void MCU1_DRAM_TEXT L2_TB_RecoverPbit_Init();

extern void L2_FtlWriteLocal(PhysicalAddr* pAddr, U32* pPage, U32* pSpare, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, XOR_PARAM *pXorParam);

void MCU1_DRAM_TEXT L2_LLFInit()
{
    U32 ulSuperPu;

    for (ulSuperPu = 0; ulSuperPu < SUBSYSTEM_SUPERPU_NUM; ulSuperPu++)
    {
        FinishedPMTIIndex[ulSuperPu] = 0;
    }

    gL2LLFStatus = L2_LLF_RECOVER_PBIT;

    L2_TB_RecoverPbit_Init();
    L2_BM_LLF_Init_BlkSN();

    return;
}

void MCU1_DRAM_TEXT L2_LLF_PMT_Init(void)
{
    U8 i;
    U16 ucSuperPu, ucLunInSuperPu;

    L2_LLFPMT();

    L2_InitPMTI();
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        L2_LLFPuInfo(i);//after PBIT LLF
    }

    L2_LLFGCManager();

#ifndef SUBSYSTEM_BYPASS_LLF_BOOT    
    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][0] = SUBSYSTEM_STATUS_SUCCESS;
        }
#ifdef PMT_ITEM_SIZE_REDUCE
        HAL_DMAESetValue((U32)(g_PMTManager->m_pPMTPage[ucSuperPu][0]), SUPER_PAGE_SIZE, INVALID_8F);
#else
		HAL_DMAESetValue((U32)(&g_PMTManager->m_pPMTPage[ucSuperPu][0]->m_PMTItems[0]), SUPER_PAGE_SIZE, INVALID_8F);
#endif

    }
#else
    for (ucSuperPu= 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {

            U32 ulPMTIndexInPu;
            for(ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; ulPMTIndexInPu ++)
            {
                g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][ulPMTIndexInPu] = SUBSYSTEM_STATUS_SUCCESS;
                HAL_DMAESetValue((U32)(&g_PMTManager->m_pPMTPage[ucSuperPu][ulPMTIndexInPu]->m_PMTItems[0]), BUF_SIZE, INVALID_8F);   
            }
        }

    }
#endif

    return;
}


U32 MCU1_DRAM_TEXT L2_LLF_SavePMT(U8 ucSuperPu)
{
    PhysicalAddr Addr = { 0 };
    RED Spare;
    U16 uPMTIIndexInPu;
    U16 BlkSN;
    U16 usCurPPO;
    U8 ucLunInSuperPu, ucTLun;
    U32 ulCmdRet;
    L2_PMT_SAVE_STAGE* pPMTSaveStage = &(g_PMTManager->m_PMTSaveStage[ucSuperPu]);
    U8* pStatus;
	U32 EraseStructSize;
	U8 *TempAddr;

    switch(*pPMTSaveStage)
    {
        case L2_PMT_SAVE_PREPARE:
            *pPMTSaveStage = L2_PMT_SAVE_WRITE_PAGE;

        case L2_PMT_SAVE_WRITE_PAGE:

            BlkSN = L2_GetCurBlockSNOfPMT(ucSuperPu);
            usCurPPO = L2_GetCurPPOOfPMT(ucSuperPu);

            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

                if (FALSE == L2_FCMDQNotFull(ucTLun))
                {
                    continue;
                }

                if (0 != (g_PMTManager->m_PMTBitMap[ucSuperPu] & (1 << ucLunInSuperPu)))
                {
                    continue;
                }

                //Flush Page Addr 
                Addr.m_PUSer = ucSuperPu;
                Addr.m_BlockInPU = L2_RT_GetAT1BlockPBNFromSN(ucSuperPu, ucLunInSuperPu, (U8)BlkSN);
                Addr.m_PageInBlock = usCurPPO;
                Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                Addr.m_LPNInPage = 0;

                uPMTIIndexInPu = FinishedPMTIIndex[ucSuperPu];

                if (0 == g_PMTManager->m_PMTBitMap[ucSuperPu])
                {
                    //L2_ClearDirty(ucSuperPu, uPMTIIndexInPu);
                    L2_IncTimeStampInPu(ucSuperPu);
                }

                Spare.m_RedComm.ulTimeStamp = L2_GetTimeStampInPu(ucSuperPu);
                Spare.m_RedComm.ulTargetOffsetTS = L2_GetTargetOffsetTS(&g_PMTManager->m_PMTBitMap[ucSuperPu]);
                Spare.m_RedComm.bsVirBlockAddr = Addr.m_BlockInPU;
                Spare.m_RedComm.bcPageType = PAGE_TYPE_PMT;
                Spare.m_RedComm.bcBlockType = BLOCK_TYPE_PMT;
				Spare.m_PMTIndex = uPMTIIndexInPu;
                COM_MemCpy(Spare.m_DirtyBitMap, g_PMTI[ucSuperPu]->m_PMTDirtyBitMapInCE.m_DirtyBitMap, PMT_DIRTY_BM_SIZE);

#ifdef ValidLPNCountSave_IN_DSRAM1
                U24setValue(ucSuperPu, uPMTIIndexInPu, 0); 
#else
                g_PMTManager->m_PMTSpareBuffer[ucSuperPu][uPMTIIndexInPu]->m_ValidLPNCountSave = 0;
#endif
                Spare.m_ValidLPNCountSave = 0;

                pStatus = &g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][uPMTIIndexInPu];

                pRecordBlkSNs[ucSuperPu] = BlkSN;
                pRecordPPOs[ucSuperPu] = usCurPPO;
                pRecordPMTIIndexs[ucSuperPu] = uPMTIIndexInPu;

                if (ucLunInSuperPu == (LUN_NUM_PER_SUPERPU - 1))
                {
                    EraseStructSize = sizeof(RecordEraseInfo);
                    TempAddr = (U8*)((U32)g_PMTManager->m_pPMTPage[ucSuperPu][0] + ucLunInSuperPu*BUF_SIZE + BUF_SIZE - EraseStructSize);
                    L2_ClearEraseInfo(ucSuperPu);
                    COM_MemByteCopy(TempAddr, (U8 *)&g_NeedEraseBlkInfo[ucSuperPu], EraseStructSize);
                }


                /* update PBIT 1st page Info */
                if (0 == Addr.m_PageInBlock)
                {
                    L2_Set_TableBlock_PBIT_Info(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, &Spare);
                }

#ifdef PMT_ITEM_SIZE_REDUCE
                L2_FtlWriteLocal(&Addr, (U32*)((U32)g_PMTManager->m_pPMTPage[ucSuperPu][0] + ucLunInSuperPu*BUF_SIZE),
                                 (U32*)&Spare,pStatus, TRUE, TRUE, NULL);
#else
				L2_FtlWriteLocal(&Addr, (U32*)((U32)&g_PMTManager->m_pPMTPage[ucSuperPu][0]->m_Page + ucLunInSuperPu*BUF_SIZE),
					(U32*)&Spare,pStatus, TRUE, TRUE, NULL);
#endif

                //FIRMWARE_LogInfo("LLF SavePMT Pu %d Blk %d Pg %d\n", ucSuperPu, Addr.m_BlockInPU, Addr.m_PageInBlock);

    #ifdef L2MEASURE
                L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_AT1_SLC);
    #endif

                g_PMTManager->m_PMTBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);

                L2_UpdatePhysicalAddr(ucSuperPu, ucLunInSuperPu, uPMTIIndexInPu, &Addr);

            }

            if (SUPERPU_LUN_NUM_BITMSK == g_PMTManager->m_PMTBitMap[ucSuperPu])
            {
                L2_ClearDirty(ucSuperPu, uPMTIIndexInPu);
                *pPMTSaveStage = L2_PMT_SAVE_WAIT_WRITE_PAGE;
            }
            break;
  
        case L2_PMT_SAVE_WAIT_WRITE_PAGE:
            uPMTIIndexInPu = FinishedPMTIIndex[ucSuperPu];

            if (L2_SAVE_PENDING == L2_CheckFlushPMTStatus(ucSuperPu, uPMTIIndexInPu))
            {
                return L2_LLFPending;
            }
            else if (L2_SAVE_SUCCESS == L2_CheckFlushPMTStatus(ucSuperPu, uPMTIIndexInPu))
            {
                *pPMTSaveStage = L2_PMT_SAVE_SUCCESS;
            }
            else
            {
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    pStatus = &g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][uPMTIIndexInPu];

                    if (SUBSYSTEM_STATUS_FAIL == *pStatus)
                    {
                        *pPMTSaveStage = L2_PMT_SAVE_FAIL_MOVE_BLOCK;

                        /* Use PMTBitMap to indicate save fail LUN*/
                        g_PMTManager->m_PMTBitMap[ucSuperPu] &= ~(1 << ucLunInSuperPu);

                        DBG_Printf("L2_LLF_SavePMT fail,SuperPu %d LUN %d PMTIIndexInPu=%d, BlockSN=%d, PPO=%d\n", ucSuperPu, ucLunInSuperPu,
                            pRecordPMTIIndexs[ucSuperPu], pRecordBlkSNs[ucSuperPu], pRecordPPOs[ucSuperPu]);

                    }
                }
            }
            break;
        case L2_PMT_SAVE_FAIL_MOVE_BLOCK:

            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (0 != (g_PMTManager->m_PMTBitMap[ucSuperPu] & (1 << ucLunInSuperPu)))
                {
                    continue;
                }

                /* save PMT write page fail: errorhandling move block */
                BlkSN = L2_GetCurBlockSNOfPMT(ucSuperPu);
                usCurPPO = L2_GetCurPPOOfPMT(ucSuperPu);

                ulCmdRet = L2_TableErrorHandlingMoveBlock(ucSuperPu, ucLunInSuperPu, L2_RT_GetAT1BlockPBNFromSN(ucSuperPu, ucLunInSuperPu, (U8)BlkSN), usCurPPO, WRITE_ERR);

                if (L2_ERR_HANDLING_PENDING == ulCmdRet)
                {
                    ;
                }
                else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
                {
                    /* Rewrite Save fail Lun, use g_PMTManager->m_PMTBitMap to indicate redo LUN*/
                    *pPMTSaveStage = L2_PMT_SAVE_WRITE_PAGE;
                }
                else
                {
                    DBG_Printf("L2_LLF_SavePMT PU 0x%x LUN 0x%x Write ErrorHandling Fail\n", ucSuperPu, ucLunInSuperPu);
                    DBG_Getch();
                }
            }
            break;

        case L2_PMT_SAVE_SUCCESS:

            g_PMTManager->m_PMTBitMap[ucSuperPu] = 0;
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
            L2_IncTimeStampInPu(ucSuperPu);
#endif
            L2_IncCurPOOfPMT(ucSuperPu);
            FinishedPMTIIndex[ucSuperPu]++;

            if (FinishedPMTIIndex[ucSuperPu] >= PMTPAGE_CNT_PER_SUPERPU)
            {
                *pPMTSaveStage = L2_PMT_SAVE_DONE;
                return L2_LLFSuccess;
            }
            else
            {
                *pPMTSaveStage = L2_PMT_SAVE_WRITE_PAGE;
            }
            break;

        case L2_PMT_SAVE_DONE:
            return L2_LLFSuccess;
            break;

        default:
            break;

    }
    return L2_LLFPending;
}

U32 MCU1_DRAM_TEXT L2_LLF_PMT(void)
{
    U8 ucSuperPu;
    U32 ulCmdRet[SUBSYSTEM_SUPERPU_MAX];
    U8 ucCheckSuperPuCnt = 0;

    if (0 == g_usInitPMTStatus)
    {
        L2_LLF_PMT_Init();
        g_usInitPMTStatus = 1;
        PMTPageCnt = 0;
    }

    /*Build all PMTPages*/
    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        ulCmdRet[ucSuperPu] = L2_LLF_SavePMT(ucSuperPu);

        if (L2_LLFPending != ulCmdRet[ucSuperPu])
        {
            ucCheckSuperPuCnt++;
        }
    }

    if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
    {
        L2_ResetSavePMTStage();
        return L2_LLFSuccess;
    }
    else
    {
        return L2_LLFPending;
    }
}


U32 MCU1_DRAM_TEXT L2_LLF(BOOL bKeepEraseCnt, BOOL bSecurityErase)
{
    U32 ulRet;
    U32 ulCmdRet;

    ulRet = L2_LLFPending;

    switch (gL2LLFStatus)
    {

    case L2_LLF_RECOVER_PBIT:
    {
        if (TRUE == L2_TB_RecoverPbit())
        {
            gL2LLFStatus = L2_LLF_START;
        }
    }
        break;

    case L2_LLF_START:
    {
        /* Init global values */
        pRT->m_DevParam.PowerCycleCnt++;
        gL2LLFStatus = L2_LLF_BBTBUILD;
    }
        break;

    case L2_LLF_BBTBUILD:
    {
        if (TRUE == L2_BbtBuild(bSecurityErase))
        {
            gL2LLFStatus = L2_LLF_RT_AT0_TABLE;
        }
    }
        break;

    case L2_LLF_RT_AT0_TABLE:
    {
        /* LLF RT, PBIT and VBT in DRAM */
        L2_TableBlock_LLF(bKeepEraseCnt);

        gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
        gL2LLFStatus = L2_LLF_SAVE_BBT;

    }
        break;
    case L2_LLF_SAVE_BBT:
    {
        /* save BBT for root pointers */
        if (TRUE == L2_BbtSave(INVALID_2F, INVALID_2F))  // check with henry or jason later //tobey
        {
            gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
            gL2LLFStatus = L2_LLF_PMT_TABLE;
        }
    }
        break;
    case L2_LLF_PMT_TABLE:
    {
        /* LLF PMT and other L2 Tables */
        ulCmdRet = L2_LLF_PMT();
        if (L2_LLFPending == ulCmdRet)
        {
            ;
        }
        else if (L2_LLFFail == ulCmdRet)
        {
            gL2LLFStatus = L2_LLF_FAIL;
        }
        else
        {
            gL2LLFStatus = L2_LLF_SAVE_TABLE;
            g_usInitPMTStatus = 0;
        }
    }
        break;

    case L2_LLF_SAVE_TABLE:
    {
        /* Save RT and AT0 Tables to Flash */
        ulCmdRet = L2_TableBlock_Shutdown();
        if (L2_TB_WAIT == ulCmdRet)
        {
            ;
        }
        else if (L2_TB_SUCCESS == ulCmdRet)
        {
            gL2LLFStatus = L2_LLF_SUCCESS;
        }
        else
        {
            gL2LLFStatus = L2_LLF_FAIL;
        }
    }
        break;

    case L2_LLF_FAIL:
    {
        gL2LLFStatus = L2_LLF_START;
        ulRet = L2_LLFFail;
    }
        break;

    case L2_LLF_SUCCESS:
    {
        gL2LLFStatus = L2_LLF_START;
        ulRet = L2_LLFSuccess;
    }
        break;

    default:
    {
        DBG_Printf("L2_LLF invalid Status 0x%x\n", gL2LLFStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}



