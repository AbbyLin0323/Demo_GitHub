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
  File Name     : L2_SimTablecheck.c
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
#ifdef SIM
#include "L2_SimTablecheck.h"
#include "HAL_FlashChipDefine.h"
#include "L2_PBIT.h"
#include "L2_VBT.h"
#include "L2_PMTPage.h"
#include "L2_PMTManager.h"
#include "L2_Boot.h"
#include "L2_RT.h"
#include "L1_SpecialSCMD.h"
#include "COM_Memory.h"
#include "HostModel.h"
#include "sim_nfccmd.h"
#include "action_define.h"
#include "xt_library.h"
#include "L2_StaticWL.h"
#include "L2_TableBBT.h"
#include "L2_FTL.h"

GLOBAL MCU12_VAR_ATTR PuInfo* dbg_g_PuInfo[SUBSYSTEM_LUN_MAX];
GLOBAL MCU12_VAR_ATTR FTL_ERASE_STATUS_MGR *dbg_g_ptFTLEraseStatusManager;
GLOBAL MCU12_VAR_ATTR PMTI* dbg_g_PMTI[SUBSYSTEM_LUN_MAX];
GLOBAL MCU12_VAR_ATTR PMTBlkManager* dbg_p_m_PMTBlkManager[SUBSYSTEM_LUN_MAX];  
GLOBAL MCU12_VAR_ATTR PBIT *dbg_pPBIT[SUBSYSTEM_LUN_MAX];
GLOBAL MCU12_VAR_ATTR VBT *dbg_pVBT[SUBSYSTEM_LUN_MAX];
GLOBAL MCU12_VAR_ATTR U32 * DbgForFreeBlockCnt;
GLOBAL MCU12_VAR_ATTR WL_INFO *dbg_gwl_info;
#ifdef DBG_LC
GLOBAL MCU12_VAR_ATTR LC   *dbg_lc;
#endif

BOOL g_bTableRebuildFlag[FW_MCU_COUNT] = { FALSE };
BOOL g_aRebuildDirtyCntFlag[FW_MCU_COUNT] = { FALSE };

struct SimRstLPNOfPMTCheck g_SimRstLPNOfPMTCheck = { 0 };
GLOBAL MCU12_VAR_ATTR struct SimRstLPNOfPMTCheck g_SimCollectErrAddr = { 0 };

extern GLOBAL Err_Info_Manager* l_ErrManagerDptr;
extern BOOL g_bTableRebuildFlag[FW_MCU_COUNT];
extern BOOL g_aRebuildDirtyCntFlag[];

extern void  NFC_CalcTimeStamp(U32 ulSuperPU, U32 ulLUNInSuperPU, U32* pMaxTS);
extern U8 Sim_GetTableRebuildType(void);
extern void NFC_ReadRedData(FLASH_PHY* pFlash_phy, RED* pRed);
extern void NFC_SetFlashAddr(FLASH_PHY* pFlash_phy, U8 Pu, U16 PhyBlk, U16 PhyPage);
extern U16 GetPPOFromPhyPage(U16 PhyPage);
extern U16  L2_VBT_GetPhysicalBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN);

U32 GetSystemLPN(U32 SubSystemLpn)
{
    U32 ulSystemLpn;
    U32 SubSystemLba, ulSystemLba;
    U32 ulSubSysNumBits;
    U32 ulMcuID = XT_RSR_PRID();

    ulSubSysNumBits = 0;

    SubSystemLba = SubSystemLpn*SEC_PER_LPN;

    ulSystemLba = (SubSystemLba & SEC_PER_BUF_MSK) + ((SubSystemLba >> SEC_PER_BUF_BITS) << (SEC_PER_BUF_BITS + ulSubSysNumBits))
        + ((ulMcuID - 2) << SEC_PER_BUF_BITS);

    ulSystemLpn = ulSystemLba / SEC_PER_LPN;

    return ulSystemLpn;
}

BOOL IsDuringTableRebuild()
{
    BOOL bRet = FALSE;
    U32 ulMCUIndex;

    for (ulMCUIndex = 0; ulMCUIndex < FW_MCU_COUNT; ulMCUIndex++)
    {
        if (TRUE == g_bTableRebuildFlag[ulMCUIndex])
        {
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}
void ResetTableRebuildFlag()
{
	BOOL bRet = FALSE;
	U32 ulMCUIndex;

	for (ulMCUIndex = 0; ulMCUIndex < FW_MCU_COUNT; ulMCUIndex++)
	{
		g_bTableRebuildFlag[ulMCUIndex] = FALSE;
	}
}

BOOL IsDuringRebuildDirtyCnt(U32 ulMcuId)
{
    BOOL bRet = FALSE;

    if (TRUE == g_aRebuildDirtyCntFlag[ulMcuId - 2])
    {
        bRet = TRUE;
    }

    return bRet;
}
BOOL L2_CheckIgnoreErrAddr(U32 LPN)
{
    U32 i;
    PhysicalAddr ErrAddr = { 0 };
    U32 ulHalPhyPu;

    for (i = 0; i < g_SimRstLPNOfPMTCheck.ErrAddrCnt; i++)
    {
        ErrAddr = g_SimRstLPNOfPMTCheck.ErrAddr[i];
        ulHalPhyPu = HAL_NfcGetPhyPU(g_DebugPMT[LPN].m_PUSer);

        if (ulHalPhyPu == ErrAddr.m_PUSer &&
            g_DebugPMT[LPN].m_BlockInPU == ErrAddr.m_BlockInPU &&
            g_DebugPMT[LPN].m_PageInBlock == ErrAddr.m_PageInBlock)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void L2_AdjustDebugPMT(U32 LPN)
{
    g_DebugPMT[LPN].m_PPN = INVALID_8F;
}

BOOL L2_CheckRebuildErrAdr(U32 LPN)
{
    U32 i;
    PhysicalAddr ErrAddr = { 0 };
    for (i = 0; i < l_ErrManagerDptr->m_Cnt; i++)
    {
        ErrAddr = l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr;

        if (g_DebugPMT[LPN].m_PUSer == ErrAddr.m_PUSer &&
            g_DebugPMT[LPN].m_BlockInPU == ErrAddr.m_BlockInPU &&
            g_DebugPMT[LPN].m_PageInBlock == ErrAddr.m_PageInBlock)
        {
            L2_AdjustDebugPMT(LPN);
            return TRUE;
        }
    }

    for (i = 0; i < g_SimCollectErrAddr.ErrAddrCnt; i++)
    {
        ErrAddr = g_SimCollectErrAddr.ErrAddr[i];

        if (g_DebugPMT[LPN].m_PUSer == ErrAddr.m_PUSer &&
            g_DebugPMT[LPN].m_BlockInPU == ErrAddr.m_BlockInPU &&
            g_DebugPMT[LPN].m_PageInBlock == ErrAddr.m_PageInBlock)
        {
            L2_AdjustDebugPMT(LPN);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL L2_IsNeedIgnoreCheck(U32 LPN, PhysicalAddr* pAddr)
{
    if (g_InjErrParamType == EXTERNAL_FLASH_PARAM)
    {
        return L2_CheckIgnoreErrAddr(LPN);
    }
    else if (g_InjErrParamType == INTERNAL_FLASH_PARAM)
    {
        return L2_CheckRebuildErrAdr(LPN);
    }
    else
    {
        PhysicalAddr Addr1 = { 0 };

        Addr1.m_PPN = g_DebugPMT[LPN].m_PPN;

        if (Addr1.m_PUSer == pAddr->m_PUSer
            && Addr1.m_BlockInPU == pAddr->m_BlockInPU
            && (Addr1.m_PageInBlock + 1) == pAddr->m_PageInBlock)
        {
            return TRUE;
        }

        return FALSE;
    }
}

void L2_CheckPMT()
{
    U32 ulLpn;
    PhysicalAddr Addr = { 0 };
    for (ulLpn = 0; ulLpn < MAX_LPN_IN_DISK; ulLpn++)
    {
        L2_LookupPMT(&Addr, ulLpn,FALSE);       
    }

    DBG_Printf("DBG_PMT check right \n");
}

void L2_RecordPMT()
{
    U32 ulLpn;
    PhysicalAddr Addr = { 0 };
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage;
#else
	PMTPage* pPMTPage;
#endif
    U32 LPNInPMTPage;
    U16 PMTIIndex = INVALID_4F;
    BOOL bPMTPageInRAM = FALSE;
    BOOL bPMTPageBuilt = FALSE;


    for (ulLpn = 0; ulLpn < MAX_LPN_IN_DISK + LPN_PER_BUF; ulLpn++)
    {
        pPMTPage = GetPMTPage(ulLpn);
        LPNInPMTPage = L2_GetOffsetInPMTPage(ulLpn);

#ifdef PMT_ITEM_SIZE_3BYTE
        L2_PMTItemToPhyAddr(&Addr, &pPMTPage->m_PMTItems[LPNInPMTPage]);
        if (INVALID_8F != Addr.m_PPN)
        {
            Addr.m_PUSer = L2_GetSuperPuFromLPN(ulLpn);
        }
#elif defined(PMT_ITEM_SIZE_REDUCE)
		L2_PMTItemToPhyAddr(&Addr, pPMTPage, LPNInPMTPage);
#else
        Addr.m_PPN = pPMTPage->m_PMTItems[LPNInPMTPage];  
#endif

        g_DebugPMT[ulLpn].m_PPN = Addr.m_PPN;
    }
}

void L2_CollectErrAddr(U8 ucSubSystemPu, U16 VirBlk, U16 Pg)
{
    U32 ErrAddrCnt;
    PhysicalAddr ErrAddr = { 0 };
    U32 i;

    if (g_SimCollectErrAddr.ErrAddrCnt > (IGNORE_LPN_CNT / LPN_PER_BUF))
    {
        DBG_Printf("g_SimRstLPNOfPMTCheck ErrAddrCnt Overflow!\n");
        DBG_Getch();
    }
    ErrAddr.m_PPN = INVALID_8F;

    ErrAddr.m_PUSer = ucSubSystemPu;
    ErrAddr.m_BlockInPU = VirBlk;
    ErrAddr.m_PageInBlock = Pg;

    ErrAddrCnt = g_SimCollectErrAddr.ErrAddrCnt;

    for (i = 0; i < ErrAddrCnt; i++)
    {
        if (ErrAddr.m_PPN == g_SimCollectErrAddr.ErrAddr[i].m_PPN)
            return;
    }

    g_SimCollectErrAddr.ErrAddr[ErrAddrCnt] = ErrAddr;
    
    DBG_Printf("L2_CollectErrAddr ucSubSystemPu %d VirBlk %d Pg %d\n", ucSubSystemPu, VirBlk, Pg);

    g_SimCollectErrAddr.ErrAddrCnt++;
}
void L2_ResetCollectErrAddr()
{
    U32 i;

    g_SimCollectErrAddr.LPNCnt = 0;
    for (i = 0; i < IGNORE_LPN_CNT; i++)
    {
        g_SimCollectErrAddr.IgnoredLPN[i] = INVALID_8F;
    }

    g_SimCollectErrAddr.ErrAddrCnt = 0;
    for (i = 0; i < IGNORE_LPN_CNT / LPN_PER_BUF; i++)
    {
        g_SimCollectErrAddr.ErrAddr[i].m_PPN = INVALID_8F;
    }
}

void L2_CollectErrAddrOfCheckPMT(U8 ucSubSystemPu, U16 PhyBlk, U16 PhyPage)
{
    U16 VirBlk = INVALID_4F;
    U16 usPPO = INVALID_4F;
    RED red;
    U8 ucHalPhyPu;
    FLASH_PHY flash_phy[PLN_PER_LUN];

    ucHalPhyPu = HAL_NfcGetPhyPU(ucSubSystemPu);

    NFC_SetFlashAddr(&flash_phy[0], ucHalPhyPu, PhyBlk, PhyPage);
    NFC_ReadRedData(&flash_phy[0], &red);

    VirBlk = red.m_RedComm.bsVirBlockAddr;
    if (VirBlk != INVALID_4F && (U8)red.m_RedComm.bcPageType == PAGE_TYPE_DATA)
    {
        if (BLOCK_TYPE_SEQ == red.m_RedComm.bcBlockType || BLOCK_TYPE_RAN == red.m_RedComm.bcBlockType)
        {
            usPPO = GetPPOFromPhyPage(flash_phy[0].nPage);
            if (INVALID_4F == usPPO)
            {
                DBG_Getch();
            }
        }
        else
        {
            usPPO = flash_phy[0].nPage;
        }
        L2_CollectErrAddr(ucSubSystemPu, VirBlk, usPPO);
    }

    return;
}

void L2_AddErrAddrOfCheckPMT(U8 PUSer, U16 VirBlk, U16 Pg)
{
    U32 ErrAddrCnt;
    PhysicalAddr ErrAddr = { 0 };
    U32 i;

    if (g_SimRstLPNOfPMTCheck.ErrAddrCnt > (IGNORE_LPN_CNT / LPN_PER_BUF))
    {
        DBG_Printf("g_SimRstLPNOfPMTCheck ErrAddrCnt Overflow!\n");
        DBG_Getch();
    }
    ErrAddr.m_PPN = INVALID_8F;

    ErrAddr.m_PUSer = PUSer;
    ErrAddr.m_BlockInPU = VirBlk;
    ErrAddr.m_PageInBlock = Pg;

    ErrAddrCnt = g_SimRstLPNOfPMTCheck.ErrAddrCnt;

    for (i = 0; i < ErrAddrCnt; i++)
    {
        if (ErrAddr.m_PPN == g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PPN)
            return;
    }

    g_SimRstLPNOfPMTCheck.ErrAddr[ErrAddrCnt] = ErrAddr;
    //DBG_Printf("AddErrAddrOfCheckPMT CE %d VirBlk %d Pg %d\n",PUSer,VirBlk,Pg);
    g_SimRstLPNOfPMTCheck.ErrAddrCnt++;
}


void L2_ResetDbgPMTByPhyAdrr(U8 PUSer, U16 PhyBlk, U16 PhyPage)
{
    FLASH_PHY FlashAddr[PLN_PER_LUN] = { 0 };
    U16 VirBlk = INVALID_4F;
    U8 ucHalPhyPu = HAL_NfcGetPhyPU(PUSer);
    U8 ucLPNIndex = 0;
    RED red;
    
    NFC_SetFlashAddr(&FlashAddr[0], ucHalPhyPu, PhyBlk, PhyPage);
    NFC_ReadRedData(&FlashAddr[0], &red);
    VirBlk = red.m_RedComm.bsVirBlockAddr;

    if (PG_TYPE_DATA_SIMNFC == red.m_RedComm.bcPageType)
    {
        for (ucLPNIndex = 0; ucLPNIndex < LPN_PER_BUF; ucLPNIndex++)
        {
            U32 ulLPN = red.m_DataRed.aCurrLPN[ucLPNIndex];

            if (INVALID_8F != ulLPN &&
                g_DebugPMT[ulLPN].m_PUSer == PUSer &&
                g_DebugPMT[ulLPN].m_BlockInPU == VirBlk &&
                g_DebugPMT[ulLPN].m_PageInBlock == PhyPage)
            {
                g_DebugPMT[ulLPN].m_PPN = INVALID_8F;
            }
        }
    }

    return;
}

void L2_RemoveErrAddrOfCheckPMT(U8 PUSer, U16 VirBlk)
{
    PhysicalAddr ErrAddr = { 0 };
    U32 i;
    U32 RemoveIndex = INVALID_8F;
    U32 ErrAddrCnt;

    ErrAddrCnt = g_SimRstLPNOfPMTCheck.ErrAddrCnt;
    if (ErrAddrCnt == 0)
        return;

    ErrAddr.m_PPN = INVALID_8F;

    ErrAddr.m_PUSer = PUSer;
    ErrAddr.m_BlockInPU = VirBlk;

    for (i = 0; i < ErrAddrCnt; i++)
    {
        if ((ErrAddr.m_PUSer == g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PUSer) &&
            (ErrAddr.m_BlockInPU == g_SimRstLPNOfPMTCheck.ErrAddr[i].m_BlockInPU))
        {
            g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PPN = INVALID_8F;
            DBG_Printf("RemoveErrAddrOfCheckPMT CE %d VirBlk %d Pg %d\n", g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PUSer,
                g_SimRstLPNOfPMTCheck.ErrAddr[i].m_BlockInPU, g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PageInBlock);

            g_SimRstLPNOfPMTCheck.ErrAddrCnt--;
            break;
        }
    }

    for (i = 0; i < ErrAddrCnt; i++)
    {
        if (INVALID_8F == g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PPN)
        {
            U32 ulCrntIndex = i + 1;
            while (ulCrntIndex < ErrAddrCnt)
            {
                if (INVALID_8F != g_SimRstLPNOfPMTCheck.ErrAddr[ulCrntIndex].m_PPN)
                {
                    g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PPN = g_SimRstLPNOfPMTCheck.ErrAddr[ulCrntIndex].m_PPN;
                    g_SimRstLPNOfPMTCheck.ErrAddr[ulCrntIndex].m_PPN = INVALID_8F;
                    i++;
                }
                ulCrntIndex++;
            }
        }
   
    }
    
}
void L2_AddIgnoredLPNOfCheckPMT(U32 LPN)
{
    U32 i;
    U32 RstLPNCnt = g_SimRstLPNOfPMTCheck.LPNCnt;
    for (i = 0; i < RstLPNCnt; i++)
    {
        if (LPN == g_SimRstLPNOfPMTCheck.IgnoredLPN[i])
            return;
    }

    g_SimRstLPNOfPMTCheck.IgnoredLPN[RstLPNCnt] = LPN;
    RstLPNCnt++;
    g_SimRstLPNOfPMTCheck.LPNCnt = RstLPNCnt;

    if (RstLPNCnt > IGNORE_LPN_CNT)
    {
        DBG_Printf("RstLPNCnt> %d \n", IGNORE_LPN_CNT);
        DBG_Getch();
    }
}

void L2_ResetIgnoredLPNOfCheckPMT()
{
    U32 i;

    g_SimRstLPNOfPMTCheck.LPNCnt = 0;
    for (i = 0; i < IGNORE_LPN_CNT; i++)
    {
        g_SimRstLPNOfPMTCheck.IgnoredLPN[i] = INVALID_8F;
    }

    g_SimRstLPNOfPMTCheck.ErrAddrCnt = 0;
    for (i = 0; i < IGNORE_LPN_CNT / LPN_PER_BUF; i++)
    {
        g_SimRstLPNOfPMTCheck.ErrAddr[i].m_PPN = INVALID_8F;
    } 
}
BOOL L2_IsIgnoredLPNOfCheckPMT(U32 LPN)
{
    BOOL Ret = FALSE;
    U32 i;

    for (i = 0; i < IGNORE_LPN_CNT; i++)
    {
        if (LPN == g_SimRstLPNOfPMTCheck.IgnoredLPN[i])
        {
            Ret = TRUE;
            break;
        }
    }

    return Ret;
}
void L2_ResetIgnoredLPN_WriteCnt()
{
    U32 i;
    U32 lba;
    U32 j;

    for (i = 0; i < g_SimRstLPNOfPMTCheck.LPNCnt; i++)
    {
        if (g_SimRstLPNOfPMTCheck.IgnoredLPN[i] != INVALID_8F)
        {
            lba = g_SimRstLPNOfPMTCheck.IgnoredLPN[i] << SEC_PER_LPN_BITS;

            for (j = 0; j < SEC_PER_LPN; j++)
            {
                Host_SaveDataCnt(lba, 0);
                lba++;
            }
        }
    }

}

void L3_RecordPBIT()
{
    U32 PUSer;
    U32 Cnt = 0;
    U8 ucBlkType;

    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        L2_PBIT_Save_UpdateData(PUSer);    
        for (ucBlkType = 0; ucBlkType < BLKTYPE_ALL; ucBlkType++)
        {
            Cnt += pPBIT[PUSer]->m_TotalDataBlockCnt[ucBlkType] - pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType];
    }
        COM_MemCpy((U32*)dbg_pPBIT[PUSer], (U32*)pPBIT[PUSer], sizeof(PBIT) / 4);
    }

    *DbgForFreeBlockCnt = Cnt;
}

void L3_RecordPuInfo()
{
    U32 i;
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        memcpy(dbg_g_PuInfo[i], g_PuInfo[i], sizeof(PuInfo));
    }
}

void L2_RecordWLInfo()
{
    memcpy(dbg_gwl_info, gwl_info, sizeof(WL_INFO));
}

void L2_RecordEraseStatusMangerInfo(void)
{
    memcpy(dbg_g_ptFTLEraseStatusManager, l_ptFTLEraseStatusManager, sizeof(FTL_ERASE_STATUS_MGR));
    return;
}

U8 L2_CalWaiteEraseStsBlockNum(U8 ucSuperPU)
{
    U8 ucWaitEraseBLKNum;
    U8 ucRecyclePtr;
    U8 ucInsertPtr;

    /* Calculate WaitErase block num */
    ucWaitEraseBLKNum = 0;
    while (ucWaitEraseBLKNum <= WAIT_ERASE_STS_QUE_SIZE)
    {
        ucRecyclePtr = dbg_g_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPU].ucRecyclePtr;
        ucInsertPtr = dbg_g_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPU].ucInsertPtr;
        if (((ucRecyclePtr + ucWaitEraseBLKNum) % WAIT_ERASE_STS_ARRAY_SIZE) == ucInsertPtr)
        {
            break;
        }
        ucWaitEraseBLKNum++;
    }

    return ucWaitEraseBLKNum;
}

U32 L2_CalWaitEraseStsBLKSLCPageCnt(U8 ucSuperPU)
{
    U32 ulEraseBLKPageCnt;
    U8 ucWaitEraseBLKNum;
    U8 ucRecyclePtr;
    U16 usEraseVBN;
    U8 ucWaitEraseBlockIdx;

    ucWaitEraseBLKNum = L2_CalWaiteEraseStsBlockNum(ucSuperPU);

    /*amount free page cnt*/
    ulEraseBLKPageCnt = 0;
    ucRecyclePtr = dbg_g_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPU].ucRecyclePtr;
    for (ucWaitEraseBlockIdx = 0; ucWaitEraseBlockIdx < ucWaitEraseBLKNum; ucWaitEraseBlockIdx++)
    {
        usEraseVBN = dbg_g_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPU].ausEraseVBN[(ucRecyclePtr + ucWaitEraseBlockIdx) % WAIT_ERASE_STS_ARRAY_SIZE];
        if (L2_IsSLCBlock(ucSuperPU, usEraseVBN))
        {
            ulEraseBLKPageCnt += SLC_PG_PER_BLK;
        }
    }

    return ulEraseBLKPageCnt;
}

void L3_RecordBlakePuInfo()
{
    U32 i;
    RPMT* pRPMT, *dbg_pRPMT;
    PuInfo* pInfo, *dbg_pInfo;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        pInfo = g_PuInfo[i];
        dbg_pInfo = dbg_g_PuInfo[i];

        //Record Seq RPMT
        pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0];
        dbg_pRPMT = dbg_pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0];

        COM_MemCpy((U32*)dbg_pRPMT, (U32*)pRPMT, sizeof(RPMT) / 4);

        //Record Rnd RPMT
        pRPMT = pInfo->m_pRPMT[TARGET_HOST_GC][0];
        dbg_pRPMT = dbg_pInfo->m_pRPMT[TARGET_HOST_GC][0];

        COM_MemCpy((U32*)dbg_pRPMT, (U32*)pRPMT, sizeof(RPMT) / 4);
    }
}

void L2_CheckRPMTByRebuild()
{
    DBG_Getch();

#if 0
    U32 PUSer;
    U32 PPO;
    PuInfo* pInfo, *dbg_pInfo;
    RPMT* pRPMT, *dbg_pRPMT;
    U32 i;
    U32 j;


    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        pInfo = g_PuInfo[PUSer];
        dbg_pInfo = dbg_g_PuInfo[PUSer];

        //Record Seq RPMT
        pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0];
        dbg_pRPMT = dbg_pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0];


            //check cE         
        if (pRPMT->m_PU != dbg_pRPMT->m_PU)
            {
            DBG_Printf("pRPMT->m_PU %d != dbg_pRPMT->m_PU %d\n", pRPMT->m_PU, dbg_pRPMT->m_PU);
                DBG_Getch();
            }

            //SeqPPO 
        PPO = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
        if (0 != PPO)
            {
            if (pRPMT->m_Block != dbg_pRPMT->m_Block)
                {
                DBG_Printf("pRPMT->m_Block %d != dbg_pRPMT->m_Block %d\n", pRPMT->m_Block, dbg_pRPMT->m_Block);
                    DBG_Getch();
                }

            for (i = 0; i < PPO; i++)
                {

                for (j = 0; j < LPN_PER_BUF; j++)
                    {
                    if (pRPMT->m_RPMTItems[i * LPN_PER_BUF + j] != dbg_pRPMT->m_RPMTItems[i * LPN_PER_BUF + j])
                        {
                            DBG_Printf("pRPMT->m_RPMTItems[i * LPN_PER_BUF + j] != dbg_pRPMT->m_RPMTItems[i * LPN_PER_BUF + j]\n");
                            DBG_Getch();
                        }
                    if (pRPMT->m_TimeStamp[i] != dbg_pRPMT->m_TimeStamp[i])
                        {
                            DBG_Printf("pRPMT->m_TimeStamp[i] != dbg_pRPMT->m_TimeStamp[i]");
                            DBG_Getch();
                        }                     
                    }
                }
            }

            //Record Rnd RPMT
        pRPMT = pInfo->m_pRPMT[TARGET_HOST_GC][0];
        dbg_pRPMT = dbg_pInfo->m_pRPMT[TARGET_HOST_GC][0];

            //RndPPO 
        PPO = pInfo->m_TargetPPO[TARGET_HOST_GC];
        if (0 != PPO)
            {
            if (pRPMT->m_Block != dbg_pRPMT->m_Block)
                {
                DBG_Printf("pRPMT->m_Block %d != dbg_pRPMT->m_Block %d\n", pRPMT->m_Block, dbg_pRPMT->m_Block);
                    DBG_Getch();
                }

            for (i = 0; i < PPO; i++)
                {

                for (j = 0; j < LPN_PER_BUF; j++)
                    {
                    if (pRPMT->m_RPMTItems[i * LPN_PER_BUF + j] != dbg_pRPMT->m_RPMTItems[i * LPN_PER_BUF + j])
                        {
                            DBG_Printf("pRPMT->m_RPMTItems[i * LPN_PER_BUF + j] != dbg_pRPMT->m_RPMTItems[i * LPN_PER_BUF + j]\n");
                            DBG_Getch();
                        }
                    if (pRPMT->m_TimeStamp[i] != dbg_pRPMT->m_TimeStamp[i])
                        {
                            DBG_Printf("pRPMT->m_TimeStamp[i] != dbg_pRPMT->m_TimeStamp[i]");
                            DBG_Getch();
                        }                     
                    }
                }        
            }
        
    }
#endif

}

void L3_RecordVBT()
{
    U32 i;
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        COM_MemCpy((U32*)dbg_pVBT[i], (U32*)pVBT[i], sizeof(VBT) / 4);
    }
}
void L2_RecordPMTI()
{
    U32 uPuNum;
    for (uPuNum = 0; uPuNum < SUBSYSTEM_SUPERPU_NUM; uPuNum++)
    {
        COM_MemCpy((U32*)dbg_g_PMTI[uPuNum], (U32*)g_PMTI[uPuNum], sizeof(PMTI) / 4);
    }
}
void L2_RecordPMTManager()
{   
    U32 i;
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        COM_MemCpy((U32*)dbg_p_m_PMTBlkManager[i], (U32*)(&g_PMTManager->m_PMTBlkManager[i]), sizeof(PMTBlkManager) / 4);
    }
}

//rand flush some PMTPage for Table rebuid check
void L2_DumpAllDirtyPMTIndex()
{
    U32 PMTDirtyCntSum = 0;
    U32 uPuNum;
    U32 PMTIndexInPu;

    //DBG_Printf("Dirty PMTIndex :");

    for (uPuNum = 0; uPuNum < SUBSYSTEM_SUPERPU_NUM; uPuNum++)
    {
        for (PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_PU; PMTIndexInPu++)
        {
            if (L2_IsPMTPageDirty(uPuNum, PMTIndexInPu))
            {
                PMTDirtyCntSum++;
            }
        }
    }

    DBG_Printf("PMTDirtyCntSum %d\n", PMTDirtyCntSum);
}

void Dbg_Record_TableRebuild_Info()
{
    //FIRMWARE_LogInfo("=====Dbg_Record_TableRebuild_Info====\n");
#ifdef DBG_TABLE_REBUILD
    L3_RecordPBIT();
    L3_RecordPuInfo();            
    L3_RecordVBT();
    L2_RecordPMTI();
    L2_RecordPMTManager();
    L2_RecordEraseStatusMangerInfo();
    L2_DumpAllDirtyPMTIndex();
    L2_RecordWLInfo();
#endif    

}

void Dbg_Record_TableInfo()
{
#ifdef DBG_TABLE_REBUILD
    L3_RecordPBIT();
    L3_RecordPuInfo();
    L2_RecordPMTI();
    L2_RecordPMTManager();
    L3_RecordVBT();
#endif 
}

void MCU1_DRAM_TEXT L2_CheckVBTMapping(U8 ucSuperPU)
{
    U16 usVirBlk, usPhyBlk;
    U32 ulInvalidVBTCnt, ulValidVirBlkCnt = 0;
    U8 ucLunInSuperPu, ucPlane, ucTlun;
    BOOL bValidMapping = TRUE;
    U16 usTargetPlaneBlock;

    for (usVirBlk = 0; usVirBlk < BLK_PER_PLN; usVirBlk++)
    {
        ulInvalidVBTCnt = 0;
        bValidMapping = TRUE;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSuperPU, ucLunInSuperPu, usVirBlk);
            if (INVALID_4F == usPhyBlk)
            {
                ulInvalidVBTCnt++;
                bValidMapping = FALSE;
            }
            else
            {
                ucTlun = L2_GET_TLUN(ucSuperPU, ucLunInSuperPu);
                for (ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
                {
                    usTargetPlaneBlock = L2_BbtGetPbnBindingTable(ucTlun, usPhyBlk, ucPlane);
                    if (usTargetPlaneBlock >= (BLK_PER_PLN + RSV_BLK_PER_PLN))
                    {
                        DBG_Printf("TLun %d blk %d_%d pln %d TargetPlaneBlock %d error\n", ucTlun, usVirBlk, usPhyBlk, ucPlane, usTargetPlaneBlock);
                        DBG_Getch();
                    }
                }
            }
        }

        if (bValidMapping)
        {
            ulValidVirBlkCnt++;
        }

        if (ulInvalidVBTCnt > 0 && ulInvalidVBTCnt != LUN_NUM_PER_SUPERPU)
        {
            DBG_Printf("SPU %d usVirBlk %d -> INVALID\n", ucSuperPU, usVirBlk);
            DBG_Getch();
        }
    }
    DBG_Printf("SPU %d ValidVirBlkCnt %d\n", ucSuperPU, ulValidVirBlkCnt);

    return;
}
void L3_CheckPuInfo()
{
    U32 i;
    U16 dbg_AllocateBlkCnt = 0;
    U8 ucBlktype;
    U32 uTotalAllocateBlkCnt = 0;
    U32 uLUNInSuperPU;
    U8 ucWaitEraseBLKNum;
    U32 ulEraseBLKPageCnt;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        dbg_AllocateBlkCnt = 0;
        uTotalAllocateBlkCnt = 0;

        if (TRUE == g_bInjectError)
        {
            U32 MaxTS = 0;
            U32 MaxTSTmp = 0;
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {                
                NFC_CalcTimeStamp( i, uLUNInSuperPU, &MaxTSTmp);
                if (MaxTSTmp > MaxTS)
                {
                    MaxTS = MaxTSTmp;
                }
            }
            
            if (MaxTS != g_PuInfo[i]->m_TimeStamp)
            {
                DBG_Printf("TimeStamp of  PuID %d err\n ", i);
                DBG_Printf("NFC Calc TimeStamp %d, new TimeStamp %d\n ", MaxTS, g_PuInfo[i]->m_TimeStamp);
                //DBG_Getch();
            }
        }
        else
        {
            //Check TimeStamp 
            if (dbg_g_PuInfo[i]->m_TimeStamp != g_PuInfo[i]->m_TimeStamp)
            {
                DBG_Printf("TimeStamp of  PuID %d err\n ", i);
                DBG_Printf("old TimeStamp %d, new TimeStamp %d\n ", dbg_g_PuInfo[i]->m_TimeStamp, g_PuInfo[i]->m_TimeStamp);
                //DBG_Getch();
            }
        }

        //Print all TLCGCBlkCnt

        //DBG_Printf("PU %d old m_TLCGCBlkCnt %d rebuild m_TLCGCBlkCnt %d \n", i, dbg_g_PuInfo[i]->m_TLCGCBlkCnt, g_PuInfo[i]->m_TLCGCBlkCnt);

        if (PG_PER_SLC_BLK != dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
        {
            if (dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] != g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
            {
                if (g_bInjectError)
                {
                    if (dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == 1 && g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == 0)
                        continue;
                }
                DBG_Printf("SeqPPO of  PuID %d err\n ", i);
                DBG_Printf("old SeqPPO %d, new SeqPPO %d\n ", dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML], g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
                DBG_Getch();
            }
            if (0 != dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
            {
                if (dbg_g_PuInfo[i]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] != g_PuInfo[i]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
                {
                    DBG_Printf("SeqBlock of  PuID %d err\n ", i);
                    DBG_Printf("old SeqBlock %d, new SeqBlock %d\n ", dbg_g_PuInfo[i]->m_TargetBlk[TARGET_HOST_WRITE_NORAML], g_PuInfo[i]->m_TargetBlk[TARGET_HOST_WRITE_NORAML]);
                    DBG_Getch();
                }
                if (dbg_g_PuInfo[i]->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML] != g_PuInfo[i]->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML])
                {
                    DBG_Printf("SeqBlock ppo bitmap err\n ");
                    DBG_Getch();
                }
            }
        }
        else
        {
            dbg_AllocateBlkCnt++;
        }

        //Check RandTarget && PPO
        if (PG_PER_SLC_BLK != dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC])
        {
            if (dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC] != g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC])
            {
                if (g_bInjectError)
                {
                    if (dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC] == 1 && g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC] == 0)
                        continue;
                }
                DBG_Printf("RndPPO of  PuID %d err\n ", i);
                DBG_Printf("old RndPPO %d, new RndPPO %d\n ", dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC], g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC]);
                DBG_Getch();
            }
            if (0 != dbg_g_PuInfo[i]->m_TargetPPO[TARGET_HOST_GC])
            {
                if (dbg_g_PuInfo[i]->m_TargetBlk[TARGET_HOST_GC] != g_PuInfo[i]->m_TargetBlk[TARGET_HOST_GC])
                {
                    DBG_Printf("RndBlock of  PuID %d err\n ", i);
                    DBG_Printf("old RndBlock %d, new RndBlock %d\n ", dbg_g_PuInfo[i]->m_TargetBlk[TARGET_HOST_GC], g_PuInfo[i]->m_TargetBlk[TARGET_HOST_GC]);
                    DBG_Getch();
                }
                if (dbg_g_PuInfo[i]->m_TargetOffsetBitMap[TARGET_HOST_GC] != g_PuInfo[i]->m_TargetOffsetBitMap[TARGET_HOST_GC])
                {
                    DBG_Printf("RndBlock ppo bitmap err\n ");
                    DBG_Getch();
                }
            }
        }
        else
        {
            dbg_AllocateBlkCnt++;
        }

        //Check DataBlockCnt
        for (ucBlktype = 0; ucBlktype < BLKTYPE_ALL; ucBlktype++)
        {
            if (dbg_g_PuInfo[i]->m_DataBlockCnt[ucBlktype] != g_PuInfo[i]->m_DataBlockCnt[ucBlktype])
            {
                DBG_Printf("DataBlockCnt of PuID %d, ucBlktype:%d err\n", i, ucBlktype);
                DBG_Printf("old DataBlockCnt %u,new DataBlockCnt %u\n", dbg_g_PuInfo[i]->m_DataBlockCnt[ucBlktype], g_PuInfo[i]->m_DataBlockCnt[ucBlktype]);
                DBG_Getch();
            }
        }

        //Check Total Blk cnt
        for (ucBlktype = 0; ucBlktype < BLKTYPE_ALL; ucBlktype++)
        {
            dbg_AllocateBlkCnt += dbg_g_PuInfo[i]->m_AllocateBlockCnt[ucBlktype];
            uTotalAllocateBlkCnt += g_PuInfo[i]->m_AllocateBlockCnt[ucBlktype];
        }

        ucWaitEraseBLKNum = L2_CalWaiteEraseStsBlockNum(i);
        ulEraseBLKPageCnt = L2_CalWaitEraseStsBLKSLCPageCnt(i);

        //Check total allocate blk cnt
        if (dbg_AllocateBlkCnt != (uTotalAllocateBlkCnt + ucWaitEraseBLKNum))
        {
            if (dbg_AllocateBlkCnt + 1 == (uTotalAllocateBlkCnt + ucWaitEraseBLKNum))
            {
                ;
            }
            else  if (dbg_AllocateBlkCnt == (uTotalAllocateBlkCnt + ucWaitEraseBLKNum) + 1)
            {
                ;
            }
            else
            {
                DBG_Printf("AllocateBlockCnt of PuID %d , Blktype:%d err\n", i, ucBlktype);
                DBG_Printf("old AllocateBlockCnt %d,new AllocateBlockCnt %d\n", dbg_AllocateBlkCnt, uTotalAllocateBlkCnt);
                DBG_Getch();
            }
        }

        //Check FreePageCnt
        if ((dbg_g_PuInfo[i]->m_SLCMLCFreePageCnt + ulEraseBLKPageCnt) != g_PuInfo[i]->m_SLCMLCFreePageCnt)
        {
            DBG_Printf("FreePageCnt of PuID %d err\n", i);
            DBG_Printf("old FreePageCnt %u,new FreePageCnt %u ulEraseBLKPageCnt %u\n", dbg_g_PuInfo[i]->m_SLCMLCFreePageCnt, g_PuInfo[i]->m_SLCMLCFreePageCnt, ulEraseBLKPageCnt);
            DBG_Getch();
        }
    }
    DBG_Printf("%s succeed\n", __FUNCTION__);
}

struct SimRstBlkOfVBTCheck g_SimRstBlkOfVBTCheck[SUBSYSTEM_SUPERPU_MAX] = { 0 };

void L3_AddIgnoredBlkOfCheckVBT(U8 CE, U16 Blk)
{
    U32 i;
    U32 RstBlkCnt = g_SimRstBlkOfVBTCheck[CE].BlkCnt;

    for (i = 0; i < RstBlkCnt; i++)
    {
        if (Blk == g_SimRstBlkOfVBTCheck[CE].IgnoredBlk[i])
            return;
    }

    g_SimRstBlkOfVBTCheck[CE].IgnoredBlk[RstBlkCnt] = Blk;
    RstBlkCnt++;
    g_SimRstBlkOfVBTCheck[CE].BlkCnt = RstBlkCnt;    

    if (RstBlkCnt > 1024)
    {
        DBG_Printf("RstBlkCnt>1024 \n");
        DBG_Getch();
    }
}

void L3_UpdateIgnoredBlkOfCheckVBT(U32* pLPN, U32 Cnt)
{
#ifdef PMT_ITEM_SIZE_REDUCE
	U32* pPMTPage;
#else
	PMTPage* pPMTPage;
#endif
    U32 LPNInPMTPage;
    PhysicalAddr Addr = { 0 };
    U32 LPN;
    U32 i;

    for (i = 0; i < Cnt; i++)
    {
        LPN = pLPN[i];
        pPMTPage = GetPMTPage(LPN);
        LPNInPMTPage = L2_GetOffsetInPMTPage(LPN);

#ifdef PMT_ITEM_SIZE_3BYTE
        L2_PMTItemToPhyAddr(&Addr, &pPMTPage->m_PMTItems[LPNInPMTPage]);
#elif defined(PMT_ITEM_SIZE_REDUCE)
		L2_PMTItemToPhyAddr(&Addr, pPMTPage, LPNInPMTPage);
#else
        Addr.m_PPN = pPMTPage->m_PMTItems[LPNInPMTPage];
#endif        

        if (INVALID_8F != Addr.m_PPN)
        {
            Addr.m_PUSer = L2_GetSuperPuFromLPN(LPN);
            L3_AddIgnoredBlkOfCheckVBT(Addr.m_PUSer, Addr.m_BlockInPU);
        }
    }    

    return;
}


BOOL L3_FindIgnoredBlkOfCheckVBT(U8 CE, U16 Blk)
{
    U32 i;
    U32 RstBlkCnt = g_SimRstBlkOfVBTCheck[CE].BlkCnt;

    for (i = 0; i < RstBlkCnt; i++)
    {
        if (Blk == g_SimRstBlkOfVBTCheck[CE].IgnoredBlk[i])
            return TRUE;
    }
    return FALSE;
}

void L3_ResetIgnoredBlkOfCheckVBT()
{
    U8 i;
    U32 j;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_SimRstBlkOfVBTCheck[i].BlkCnt = 0;

        for (j = 0; j < 1024; j++)
            g_SimRstBlkOfVBTCheck[i].IgnoredBlk[j] = 0;
    }
}

void L3_CheckVBT()
{
    U16 PUSer;
    U16 BlkSN;
    U32 PuID;
    U32 NoNeedCheck;


    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        for (BlkSN = 0; BlkSN < BLK_PER_PLN; BlkSN++)
        {
            NoNeedCheck = FALSE;
            //when ppo of rand&seq target is zero,actually it is free blk, no need check
            for (PuID = 0; PuID < SUBSYSTEM_SUPERPU_NUM; PuID++)
            {

                //seq target
                if (0 == dbg_g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
                {
                    if (BlkSN == dbg_g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
                    {
                        NoNeedCheck = TRUE;
                        break;
                    }
                }
                //rand target
                if (0 == dbg_g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_GC])
                {
                    if (BlkSN == dbg_g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_GC])
                    {
                        NoNeedCheck = TRUE;
                        break;
                    }
                }

                //seq target
                if (0 == g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
                {
                    if (BlkSN == g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
                    {
                        NoNeedCheck = TRUE;
                        break;
                    }
                }
                //rand target
                if (0 == g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_GC])
                {
                    if (BlkSN == g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_GC])
                    {
                        NoNeedCheck = TRUE;
                        break;
                    }
                }
            }

            if (TRUE == NoNeedCheck)
                continue;

            //if it is ignored blk of check VBT,ignored it
            if (L3_FindIgnoredBlkOfCheckVBT((U8)PUSer, BlkSN))
                continue;

            //Check Pu ID
            if (dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID != pVBT[PUSer]->m_VBT[BlkSN].StripID)
            {
                if (VBT_TARGET_INVALID != dbg_pVBT[PUSer]->m_VBT[BlkSN].Target)
                {
                    U32 PuID;
                    PuID = dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID;

                    DBG_Printf("PuID of CE %d ,BlkSN %d  err\n ", PUSer, BlkSN);
                    DBG_Printf("Old PuID %d, new PuID %d\n ", dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID,
                        pVBT[PUSer]->m_VBT[BlkSN].StripID);
                    DBG_Getch();
                }
            }

            if (TABLE_REBUILD_NONE == Sim_GetTableRebuildType());
            continue;

            if (dbg_pVBT[PUSer]->m_VBT[BlkSN].Target != pVBT[PUSer]->m_VBT[BlkSN].Target)
            {
                U32 PuID;

                if (VBT_TARGET_INVALID != dbg_pVBT[PUSer]->m_VBT[BlkSN].Target)
                {
                    PuID = dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID;
                }
                else if (VBT_TARGET_INVALID != pVBT[PUSer]->m_VBT[BlkSN].Target)
                {
                    PuID = pVBT[PUSer]->m_VBT[BlkSN].StripID;
                }
                else
                {
                    DBG_Printf("Unknow case \n");
                    DBG_Getch();
                }

                if (dbg_g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == PG_PER_SLC_BLK)
                {
                    if (dbg_g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] == BlkSN
                        && pVBT[PUSer]->m_VBT[BlkSN].Target == VBT_NOT_TARGET)
                        continue;
                    if (g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] == BlkSN
                        && pVBT[PUSer]->m_VBT[BlkSN].Target == VBT_TARGET_HOST_W)
                        continue;
                }
                if (dbg_g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_GC] == PG_PER_SLC_BLK)
                {
                    if (dbg_g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_GC] == BlkSN
                        && pVBT[PUSer]->m_VBT[BlkSN].Target == VBT_NOT_TARGET)
                        continue;
                    if (g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_GC] == BlkSN
                        && pVBT[PUSer]->m_VBT[BlkSN].Target == VBT_TARGET_HOST_GC)
                        continue;
                }

                DBG_Printf("Target of CE %d ,BlkSN %d  err\n ", PUSer, BlkSN);
                DBG_Printf("Old Target %d, new Target %d\n ", dbg_pVBT[PUSer]->m_VBT[BlkSN].Target,
                    pVBT[PUSer]->m_VBT[BlkSN].Target);
                DBG_Getch();
            }

        }
    }

    DBG_Printf("%s succeed\n", __FUNCTION__);
}

void L3_CheckPBIT()
{
    U32 PUSer;
    U32 Cnt = 0;
    U8 ucBlkType;
    U32 ucWaitEraseBLKNum;

    //find which blk err
    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        for (ucBlkType = 0; ucBlkType < BLKTYPE_ALL; ucBlkType++)
        {
            if (dbg_pPBIT[PUSer]->m_TotalDataBlockCnt[ucBlkType] != pPBIT[PUSer]->m_TotalDataBlockCnt[ucBlkType])
            {
                DBG_Printf("PUSer 0x%x m_TotalDataBlockCnt Blktype:%d error, Old 0x%x New 0x%x!\n", PUSer, ucBlkType,
                    dbg_pPBIT[PUSer]->m_TotalDataBlockCnt[ucBlkType], pPBIT[PUSer]->m_TotalDataBlockCnt[ucBlkType]);
                DBG_Getch();
            }

            if (dbg_pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] != pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType])
            {
                U32 ZeroPPOOCnt = 0;

                if (BLKTYPE_SLC == ucBlkType)
                {
                    if (0 == dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
                    {
                        ZeroPPOOCnt++;
                    }

                    if (0 == dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC])
                    {
                        ZeroPPOOCnt++;
                    }

                    if (dbg_pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] != (pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] + ZeroPPOOCnt))
                    {
                        /* Calculate WaitErase block num */
                        ucWaitEraseBLKNum = L2_CalWaiteEraseStsBlockNum(PUSer);

                        if ((dbg_pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] - (pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] + ZeroPPOOCnt)) == ucWaitEraseBLKNum)
                        {
                            DBG_Printf("SLC area Erase done but L2 don't have time to recycle before abnormal shutdown happen\n");
                        }
                        else
                        {

                            DBG_Printf("PUSer 0x%x m_AllocatedDataBlockCnt 1 error, BlkType:%d, Old 0x%x New 0x%x!\n", PUSer, ucBlkType,
                                dbg_pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType], pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType]);

                            DBG_Getch();
                        }
                    }

                }
                else //TLC
                {
                    if (dbg_pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] - pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] > 2)
                    {
                        ucWaitEraseBLKNum = L2_CalWaiteEraseStsBlockNum(PUSer);

                        if ((dbg_pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType] - pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType]) <= (ucWaitEraseBLKNum + 2))
                        {
                            DBG_Printf("TLC area Erase done but L2 don't have time to recycle before abnormal shutdown happen\n");
                        }
                        else
                        {
                            DBG_Printf("PUSer 0x%x m_AllocatedDataBlockCnt 2 error, BlkType:%d, Old 0x%x New 0x%x!\n", PUSer, ucBlkType,
                                dbg_pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType], pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlkType]);

                            DBG_Getch();
                        }
                    }
                }
            }
        }

        if (dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == 0 || dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == PG_PER_SLC_BLK)
        {
            if (pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML] != INVALID_4F)
            {
                DBG_Printf("PUSer 0x%x m_TargetBlk[TARGET_HOST_WRITE_NORAML] 1 error, Old 0x%x New 0x%x!\n", PUSer,
                    dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML]);
                DBG_Getch();
            }
        }
        else
        {
            if (TRUE == g_bInjectError)
            {
                if ((dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML] != pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML]) && (pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML] != INVALID_4F))
                {
                    DBG_Printf("PUSer 0x%x m_TargetBlk[TARGET_HOST_WRITE_NORAML] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML]);
                    DBG_Getch();
                }
                if ((dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] != pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]) && (pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] != INVALID_4F))
                {
                    DBG_Printf("PUSer 0x%x m_TargetPPO[TARGET_HOST_WRITE_NORAML] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML], pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
                    DBG_Getch();
                }
            }
            else
            {
                if (dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML] != pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML])
                {
                    DBG_Printf("PUSer 0x%x m_TargetBlk[TARGET_HOST_WRITE_NORAML] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML]);
                    DBG_Getch();
                }
                if (dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] != pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
                {
                    DBG_Printf("PUSer 0x%x m_TargetPPO[TARGET_HOST_WRITE_NORAML] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML], pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
                    DBG_Getch();
                }
            }
        }
        if (dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC] == 0 || dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC] == PG_PER_SLC_BLK)
        {
            if (pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC] != INVALID_4F)
            {
                DBG_Printf("PUSer 0x%x m_TargetBlk[TARGET_HOST_GC] 1 error, Old 0x%x New 0x%x!\n", PUSer,
                    dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC]);
                DBG_Getch();
            }
        }
        else
        {
            if (TRUE == g_bInjectError)
            {
                if ((dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC] != pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC]) && (pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC] != INVALID_4F))
                {
                    DBG_Printf("PUSer 0x%x m_TargetBlk[TARGET_HOST_GC] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC]);
                    DBG_Getch();
                }
                if ((dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC] != pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC]) && (pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC] != INVALID_4F))
                {
                    DBG_Printf("PUSer 0x%x m_TargetPPO[TARGET_HOST_GC] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC], pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC]);
                    DBG_Getch();
                }
            }
            else
            {
                if (dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC] != pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC])
                {
                    DBG_Printf("PUSer 0x%x m_TargetBlk[TARGET_HOST_GC] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC]);
                    DBG_Getch();
                }
                if (dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC] != pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC])
                {
                    DBG_Printf("PUSer 0x%x m_TargetPPO[TARGET_HOST_GC] error, Old 0x%x New 0x%x!\n", PUSer,
                        dbg_pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC], pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC]);
                    DBG_Getch();
                }
            }
        }
    }

    DBG_Printf("%s succeed\n", __FUNCTION__);
}

void L3_CheckPBITwithPuInfo()
{

    U32 PUSer;
    U32 Cnt = 0;
    U8 ucBlktype;


    //find which blk err
    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        for (ucBlktype = 0; ucBlktype < BLKTYPE_ALL; ucBlktype++)
        {
            if (g_PuInfo[PUSer]->m_DataBlockCnt[ucBlktype] != pPBIT[PUSer]->m_TotalDataBlockCnt[ucBlktype])
            {
                DBG_Printf("PUSer 0x%x m_DataBlockCnt error, Blktype:%d, Old 0x%x New 0x%x!\n", PUSer, ucBlktype,
                    g_PuInfo[PUSer]->m_DataBlockCnt[ucBlktype], pPBIT[PUSer]->m_TotalDataBlockCnt[ucBlktype]);
                DBG_Getch();
            }
            if (g_PuInfo[PUSer]->m_AllocateBlockCnt[ucBlktype] != pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlktype])
            {

                DBG_Printf("PUSer 0x%x m_AllocateBlockCnt Blktype:%d error, Old 0x%x New 0x%x!\n", PUSer, ucBlktype,
                    g_PuInfo[PUSer]->m_AllocateBlockCnt[ucBlktype], pPBIT[PUSer]->m_AllocatedDataBlockCnt[ucBlktype]);
                DBG_Getch();
            }
        }

        if (g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == 0)
        {

        }
        else
        {
            if (g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] != pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML])
            {
                DBG_Printf("PUSer 0x%x g_PuInfo m_TargetBlk[TARGET_HOST_WRITE_NORAML] error, Old 0x%x New 0x%x!\n", PUSer,
                    g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_WRITE_NORAML], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_WRITE_NORAML]);
                DBG_Getch();
            }

            if (g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] != pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
            {
                DBG_Printf("PUSer 0x%x g_PuInfo m_TargetPPO[TARGET_HOST_WRITE_NORAML] error, Old 0x%x New 0x%x!\n", PUSer,
                    g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML], pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
                DBG_Getch();
            }
        }


        if (g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_GC] == 0)
        {
        }
        else
        {
            if (g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_GC] != pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC])
            {
                DBG_Printf("PUSer 0x%x g_PuInfo m_TargetBlk[TARGET_HOST_GC] error, Old 0x%x New 0x%x!\n", PUSer,
                    g_PuInfo[PUSer]->m_TargetBlk[TARGET_HOST_GC], pPBIT[PUSer]->m_TargetBlock[TARGET_HOST_GC]);
                DBG_Getch();
            }


            if (g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_GC] != pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC])
            {
                DBG_Printf("PUSer 0x%x g_PuInfo m_TargetPPO[TARGET_HOST_GC] error, Old 0x%x New 0x%x!\n", PUSer,
                    g_PuInfo[PUSer]->m_TargetPPO[TARGET_HOST_GC], pPBIT[PUSer]->m_TargetPPO[TARGET_HOST_GC]);
                DBG_Getch();
            }
        }
    }

    DBG_Printf("%s succeed\n", __FUNCTION__);
}

BOOL L2_IsNeedIgnoreCheckPMTI(U8 ucSuperPU, U32 uLUNInSuperPU, U16 PMTIndexInPu)
{
    PhysicalAddr ErrAddr = { 0 };
    U32 i;
    ErrAddr.m_PPN = 0;
    BOOL bRet = FALSE;
    RED red, red2;
    FLASH_PHY flash_phy[PLN_PER_LUN];

    /* normal boot load one PMT fail, and this index PMT have no copy, before enter rebuild cause dbg_g_PMTI have one PMTI=INVALID_8F.
       when enter rebuild PMT page red read success, the rebuild g_PMTI of this index is not INVALID_8F.
       (this case enter this if branch)
    */
    if (INVALID_8F == g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PPN || INVALID_8F == dbg_g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PPN)
    {
        g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PPN = INVALID_8F;
        return TRUE;
    }

    //1.patch for error injection check
    ErrAddr.m_PUSer = dbg_g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PUSer;
    ErrAddr.m_OffsetInSuperPage = dbg_g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_OffsetInSuperPage;
    ErrAddr.m_BlockInPU = L2_RT_GetAT1BlockPBNFromSN(dbg_g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PUSer,
        dbg_g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_OffsetInSuperPage, 
        dbg_g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_BlockInPU);
    ErrAddr.m_PageInBlock = dbg_g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PageInBlock;

    for (i = 0; i < l_ErrManagerDptr->m_Cnt; i++)
    {
        if (l_ErrManagerDptr->m_Err_Info[i].m_ErrTag == PMT_PAGE_ERR)
        {
            PhysicalAddr ErrAddrTmp = ErrAddr;
            PhysicalAddr ManagerErrAddrTmp = l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr;
            ErrAddrTmp.m_OffsetInSuperPage = 0;
            ManagerErrAddrTmp.m_OffsetInSuperPage = 0;
            if (ErrAddrTmp.m_PPN == ManagerErrAddrTmp.m_PPN)
            {
                bRet = TRUE;
                break;
            }
        }
    }

    //1.another patch for error injection check
    if (FALSE == bRet)
    {
        PhysicalAddr ErrAddr2 = { 0 };
        U8 ucHalPhyPu = HAL_NfcGetPhyPU(L2_GET_TLUN(ErrAddr.m_PUSer,ErrAddr.m_OffsetInSuperPage));
        U16 PhyBlk = ErrAddr.m_BlockInPU;
#ifndef FLASH_INTEL_3DTLC
        U16 PhyPage = HAL_FlashGetSLCPage(ErrAddr.m_PageInBlock)*PG_PER_WL;
#else
        U16 PhyPage = HAL_FlashGetSLCPage(ErrAddr.m_PageInBlock);
#endif
        NFC_SetFlashAddr(&flash_phy[0], ucHalPhyPu, PhyBlk, PhyPage);
        NFC_ReadRedData(&flash_phy[0], &red);

        ErrAddr2.m_PUSer = g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PUSer;
        ErrAddr2.m_OffsetInSuperPage = g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_OffsetInSuperPage;
        ErrAddr2.m_BlockInPU = L2_RT_GetAT1BlockPBNFromSN(g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PUSer,
            g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_OffsetInSuperPage, 
            g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_BlockInPU);
        ErrAddr2.m_PageInBlock = g_PMTI[ucSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PageInBlock;

        ucHalPhyPu = HAL_NfcGetPhyPU(L2_GET_TLUN(ErrAddr2.m_PUSer, ErrAddr2.m_OffsetInSuperPage));
        PhyBlk = ErrAddr2.m_BlockInPU;
#ifndef FLASH_INTEL_3DTLC
        PhyPage = HAL_FlashGetSLCPage(ErrAddr2.m_PageInBlock)*PG_PER_WL;
#else
        PhyPage = HAL_FlashGetSLCPage(ErrAddr2.m_PageInBlock);
#endif

        NFC_SetFlashAddr(&flash_phy[0], ucHalPhyPu, PhyBlk, PhyPage);
        NFC_ReadRedData(&flash_phy[0], &red2);

        if (red.m_RedComm.bcPageType == PAGE_TYPE_PMT && red2.m_RedComm.bcPageType == PAGE_TYPE_PMT)
        {
            if (red2.m_RedComm.ulTimeStamp > red.m_RedComm.ulTimeStamp)
            {
                return TRUE;
            }
        }
    }

    return bRet;
}
U32 L2_CheckPMTI()
{
    U32 uSuperPU;
    U32 uLUNInSuperPU;
    U16 PMTIndexInPu;
    BOOL bErr = FALSE;

    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        for (PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_PU; PMTIndexInPu++)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if (g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PPN == dbg_g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PPN)
            {
                continue;
            }
            else
            {
                U32 AT1SN = g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurBlkSN;               
                if ( (dbg_g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PageInBlock == g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurPPO - 1) &&
                    (dbg_g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_BlockInPU == AT1SN) )
                {
                    DBG_Printf("L2_CheckPMTI:SuperPMT page partial write\n");
                    continue;
                }
                if (TRUE == g_bInjectError)
                {
                    if (TRUE == L2_IsNeedIgnoreCheckPMTI(uSuperPU, uLUNInSuperPU, PMTIndexInPu))
                    {
                        /* if one LUN of one SuperPage should ignore, other LUN also not check */
                        //continue;                            
                        break;
                    }
                    bErr = TRUE;
                }
                bErr = TRUE;

                if (bErr)
                {
                        DBG_Printf("SuperPU %d Table rebuild PMTI error\n", uSuperPU);
                        DBG_Printf("Old PMTIItems[%d].m_PhyAddr: SuperPU %d, SuperBlock %d, SuperPage %d, offset %d,\n", PMTIndexInPu,
                            dbg_g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PUSer,                            
                            dbg_g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_BlockInPU,
                            dbg_g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PageInBlock,
                            dbg_g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_OffsetInSuperPage);
                        DBG_Printf("Current PMTIItems[%d].m_PhyAddr: SuperPU %d, SuperBlock %d, SuperPage %d, offset %d\n", PMTIndexInPu,
                            g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PUSer,
                            g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_BlockInPU,
                            g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_PageInBlock,
                            g_PMTI[uSuperPU]->m_PhyAddr[PMTIndexInPu][uLUNInSuperPU].m_OffsetInSuperPage);

                    DBG_Getch();
                }
            }
        }
    }     
    }     
    DBG_Printf("%s succeed\n", __FUNCTION__);
    return TRUE;

}


U32 L2_CheckPMTManager()
{
    U32 uSuperPU;
    U32 uLUNInSuperPU;
    U16 OldBlkSN;
    U16 OldDirtyCnt;
    U16 CurDirtyCnt;

    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        U16 OldCurBlkSN;
        U16 OldPPO;
        BOOL bShouldSkip = FALSE;

        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            if (L2_FindErrPMTCEOfRebuild((U8)uSuperPU, uLUNInSuperPU))
            {
                bShouldSkip = TRUE;
                break;
            }                
        }        

        if (bShouldSkip)
        {
            continue;
        }

        OldCurBlkSN = dbg_p_m_PMTBlkManager[uSuperPU]->m_CurBlkSN;
        OldPPO = dbg_p_m_PMTBlkManager[uSuperPU]->m_CurPPO;


        if (g_PMTManager->m_PMTBlkManager[uSuperPU].m_FreePagesCnt != dbg_p_m_PMTBlkManager[uSuperPU]->m_FreePagesCnt)
        {
            /*add patch by Nina 2016-04-26, because L2 flush PMT add wait flashstatus*/
            if (dbg_p_m_PMTBlkManager[uSuperPU]->m_FreePagesCnt == (g_PMTManager->m_PMTBlkManager[uSuperPU].m_FreePagesCnt + 1))
            {
                continue;
            }

            /*This occur when do PMT block erase during table GC and then shutdown. erase cmd send, and L3 do successfully, but L2 have no chance to update
            g_PMTManager->m_PMTBlkManager[ucSuperPu].m_FreePagesCnt value.*/
            if (dbg_p_m_PMTBlkManager[uSuperPU]->m_FreePagesCnt == (g_PMTManager->m_PMTBlkManager[uSuperPU].m_FreePagesCnt - PG_PER_SLC_BLK))
            {
                continue;
            }

            DBG_Printf("Table rebuild PMTBlkManager err\n");
            DBG_Printf("CE %d Old PMTBlkManager FreePagesCnt %d\n", uSuperPU, dbg_p_m_PMTBlkManager[uSuperPU]->m_FreePagesCnt);
            DBG_Printf("CE %d Current PMTBlkManager FreePagesCnt %d\n", uSuperPU, g_PMTManager->m_PMTBlkManager[uSuperPU].m_FreePagesCnt);
            DBG_Getch();
        }
        //check current ppo
        if (dbg_p_m_PMTBlkManager[uSuperPU]->m_CurPPO != g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurPPO)
        {
            if (dbg_p_m_PMTBlkManager[uSuperPU]->m_CurPPO + 1 == g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurPPO)
            {
                DBG_Printf("SuperPMT Page patial write case when SPOP\n");
            }
            else
            {
                DBG_Printf("g_PMTManager->m_PMTBlkManager[PUSer %d].m_CurPPO err\n", uSuperPU);
                DBG_Getch();
            }
        }

        for (OldBlkSN = 0; OldBlkSN < AT1_BLOCK_COUNT; OldBlkSN++)
        {

            //find old used physical PMTPage blk
            if (0 == dbg_p_m_PMTBlkManager[uSuperPU]->m_PMTBlkInfo[OldBlkSN].m_bFree)
            {
                //When Current PPO ==0, acutall it's free
                if (OldBlkSN == OldCurBlkSN && 0 == OldPPO)
                {
                    continue;
                }

                //check dirty cont                
                OldDirtyCnt = dbg_p_m_PMTBlkManager[uSuperPU]->m_PMTBlkInfo[OldBlkSN].m_DirtyCnt;
                CurDirtyCnt = g_PMTManager->m_PMTBlkManager[uSuperPU].m_PMTBlkInfo[OldBlkSN].m_DirtyCnt;
                if (OldDirtyCnt != CurDirtyCnt)
                {
                    DBG_Printf("CE %d BlkSN %d OldDirtyCnt %d, CurDirtyCnt %d err\n", uSuperPU, OldBlkSN, OldDirtyCnt, CurDirtyCnt);
                    DBG_Getch();
                }                
            }

        }
    }
    DBG_Printf("%s succeed\n", __FUNCTION__);
    return TRUE;
}


void DBG_L3_CheckVBT(U32 PUSer)
{
    //U16 PUSer;
    U16 BlkSN;
    U32 PuID;
    U32 NoNeedCheck;

    for (BlkSN = 0; BlkSN < BLK_PER_PLN; BlkSN++)
    {
        NoNeedCheck = FALSE;
        //when ppo of rand&seq target is zero,actually it is free blk, no need check
        for (PuID = 0; PuID < SUBSYSTEM_SUPERPU_NUM; PuID++)
        {
            //seq target
            if (0 == dbg_g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
            {
                if (BlkSN == dbg_g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
                {
                    NoNeedCheck = TRUE;
                    break;
                }
            }
            //rand target
            if (0 == dbg_g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_GC])
            {
                if (BlkSN == dbg_g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_GC])
                {
                    NoNeedCheck = TRUE;
                    break;
                }
            }

            //seq target
            if (0 == g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
            {
                if (BlkSN == g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
                {
                    NoNeedCheck = TRUE;
                    break;
                }
            }
            //rand target
            if (0 == g_PuInfo[PuID]->m_TargetPPO[TARGET_HOST_GC])
            {
                if (BlkSN == g_PuInfo[PuID]->m_TargetBlk[TARGET_HOST_GC])
                {
                    NoNeedCheck = TRUE;
                    break;
                }
            }
        }

        if (TRUE == NoNeedCheck)
            continue;

        //if it is ignored blk of check VBT,ignored it
        if (L3_FindIgnoredBlkOfCheckVBT(PUSer, BlkSN))
            continue;

        //Check Pu ID
        if (dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID != pVBT[PUSer]->m_VBT[BlkSN].StripID)
        {
            if (VBT_TARGET_INVALID != dbg_pVBT[PUSer]->m_VBT[BlkSN].Target)
            {
                U32 PuID;
                PuID = dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID;

                DBG_Printf("PuID of CE %d ,BlkSN %d  err\n ", PUSer, BlkSN);
                DBG_Printf("Old PuID %d, new PuID %d\n ", dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID,
                    pVBT[PUSer]->m_VBT[BlkSN].StripID);
                DBG_Getch();
            }
        }

        //Check target
        if (dbg_pVBT[PUSer]->m_VBT[BlkSN].Target != pVBT[PUSer]->m_VBT[BlkSN].Target)
        {
            U32 PuID;

            if (VBT_TARGET_INVALID != dbg_pVBT[PUSer]->m_VBT[BlkSN].Target)
            {
                PuID = dbg_pVBT[PUSer]->m_VBT[BlkSN].StripID;
            }
            else if (VBT_TARGET_INVALID != pVBT[PUSer]->m_VBT[BlkSN].Target)
            {
                PuID = pVBT[PUSer]->m_VBT[BlkSN].StripID;
            }
            else
            {
                DBG_Printf("Unknow case \n");
                DBG_Getch();
            }


            DBG_Printf("Target of CE %d ,BlkSN %d  err\n ", PUSer, BlkSN);
            DBG_Printf("Old Target %d, new Target %d\n ", dbg_pVBT[PUSer]->m_VBT[BlkSN].Target,
                pVBT[PUSer]->m_VBT[BlkSN].Target);
            DBG_Getch();
        }

    }

    DBG_Printf("%s succeed\n", __FUNCTION__);

}

#endif // end #ifdef SIM
