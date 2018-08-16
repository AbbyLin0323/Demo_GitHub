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
#ifdef SIM
#include "BaseDef.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_MemoryMap.h"
#include "L2_PMTPage.h"
#include "L2_StripeInfo.h"
#include "L2_PBIT.h"
#include "L2_PMTI.h"
#include "L2_VBT.h"
#include "L2_PMTManager.h"
#include "L2_TableBlock.h"

#include "L2_TableBBT.h"

#include "flash_meminterface.h"
#include "L2_SimTablecheck.h"
#include "L2_SimRebuildTable.h"
//#include "sim_nfccmd.h"
#include "sim_flash_config.h"


GLOBAL MCU12_VAR_ATTR NFC_REBUILD_DBG_INFO* g_pNfcRebuildDbgInfo;
GLOBAL MCU12_VAR_ATTR PuInfo* g_pTempPuInfo[SUBSYSTEM_LUN_MAX][TEMP_TARGET_CNT];

extern CRITICAL_SECTION g_CHCriticalSection[];

extern U8 NFC_InterfaceGetErrHold(U8 pu);
extern NFCQ_ENTRY* Comm_GetCQEntry(U8 pu, U8 rp);
extern U8 NFC_InterfaceGetCmdType(U8 pu, U8 level);
//extern NFC_PRCQ_ENTRY * Comm_GetPRCQEntry(U8 Pu, U8 Rp);
//extern void NFC_GetCmdParam(U8 pu,ST_FLASH_CMD_PARAM *p_flash_param,NFCQ_ENTRY *p_nf_cq_entry,
//                            NFC_PRCQ_ENTRY *p_nf_prcq_entry,U8 cmd_type);
extern void Comm_ReadOtfb(U32 addr, U32 nWords, U32 *buf);
extern U32 HAL_GetMcuId();
/* global region: declare global variable                                     */
extern BOOL g_bInjectError;

// interface from NFC model
extern void NFC_GetPendingLPN(U32* pPendingLPN,U32* pLPNCnt);
extern BOOL NFC_IsAddrUECC(FLASH_PHY* pFlash_phy);
extern void NFC_InterfaceResetCQ(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 level);

/*****************************************************************************
 Prototype      : L3_GetPendingLPN
 Description    : Get CE_FIFO not Pending write LPN
 Input          : U32* pPendingLPN  
                  U32* pLPNCnt      
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2014/6/17
   Author       : NinaYang
   Modification : Created function
*****************************************************************************/
void MCU12_DRAM_TEXT L3_GetPendingLPN(U32* pPendingLPN,U32* pLPNCnt)
{
    // New L3 arch can make it no pending fcmd.
    return;
}

void NFC_SetFlashAddr(FLASH_PHY* pFlash_phy,U8 Pu,U16 PhyBlk,U16 PhyPage)
{
    U8 nPlnIndex = 0;
    FLASH_PHY* pFlash_phy_temp;
    for (nPlnIndex = 0; nPlnIndex < PLN_PER_LUN; nPlnIndex++)
    {
        pFlash_phy_temp = pFlash_phy + nPlnIndex;

        pFlash_phy_temp->bsPhyPuInTotal  = Pu;
        pFlash_phy_temp->ucLunInTotal = Pu;
        pFlash_phy_temp->nPln = nPlnIndex;
        pFlash_phy_temp->nBlock = PhyBlk;
        pFlash_phy_temp->nPage = PhyPage;
    }
}

void NFC_ReadRedData(FLASH_PHY* pFlash_phy,RED* pRed)
{
    U8 Pln;
    U16 usPhyBlk = pFlash_phy->nBlock;

    for (Pln = 0; Pln < PLN_PER_LUN; Pln++)
    {
        pFlash_phy->nBlock = L2_BbtGetPbnBindingTable(pFlash_phy->ucLunInTotal, usPhyBlk, Pln);

        Mid_Read_RedData(pFlash_phy + Pln, (char*)((U32)pRed + RED_SZ*Pln));
    }
}

BOOL IsTableBlock(U16 BlkCnt)
{
    U16 TableBlkCnt;

    TableBlkCnt = TABLE_BLOCK_COUNT;
    if(BlkCnt <= TableBlkCnt)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
BOOL IsDataBlock(U16 BlkCnt)
{
    U16 TableBlkCnt;

    TableBlkCnt = TABLE_BLOCK_COUNT;
    if(BlkCnt > TableBlkCnt)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void NFC_CalcMaxTableTS(U8 ucHalPhyPu,U16 PhyBlk,U32* pMaxTableTS)
{
    U16 PhyPage;
    FLASH_PHY flash_phy[PLN_PER_LUN];
    RED red;
    U8 pg_type;
    U32 TimeStamp;
    U16 PPO;

    for(PPO = 0; PPO < PG_PER_SLC_BLK; PPO++)
    {
#ifndef FLASH_INTEL_3DTLC
        PhyPage = HAL_FlashGetSLCPage(PPO) *PG_PER_WL;
#else
       PhyPage = HAL_FlashGetSLCPage(PPO);
#endif
        NFC_SetFlashAddr(&flash_phy[0],ucHalPhyPu,PhyBlk,PhyPage);

        NFC_ReadRedData(&flash_phy[0], &red);

        pg_type = red.m_RedComm.bcPageType;
        if (PG_TYPE_FREE == pg_type)
        {
            //read empty page,skip this block                
            break;
        }
        TimeStamp = red.m_RedComm.ulTimeStamp;

        if(TRUE == NFC_IsAddrUECC(&flash_phy[0]))
        {
            DBG_Printf("Skip ErrAddr Pu %d PhyBlk %d PhyPage %d TimeStamp %d\n",ucHalPhyPu,PhyBlk,PhyPage,TimeStamp);
            continue;
        }
        if ((pg_type >= PAGE_TYPE_RT) && (pg_type <= PAGE_TYPE_TRACE))
        {
            if(*pMaxTableTS < TimeStamp)
            {
                *pMaxTableTS = TimeStamp;
            }
        }
    }
}

void NFC_CalcSLCBlockTS(U8 ucHalPhyPu,U16 PhyBlk,U32* pMaxTS)
{
    U16 PhyPage;
    FLASH_PHY flash_phy[PLN_PER_LUN];
    RED red;
    U8 pg_type;
    U32 TimeStamp;
    U16 PPO;

    for (PPO = 0; PPO < PG_PER_SLC_BLK; PPO++)
    {
#ifndef FLASH_INTEL_3DTLC
        PhyPage = HAL_FlashGetSLCPage(PPO) *PG_PER_WL;
#else
        PhyPage = HAL_FlashGetSLCPage(PPO);
#endif
        NFC_SetFlashAddr(&flash_phy[0],ucHalPhyPu,PhyBlk,PhyPage);
        NFC_ReadRedData(&flash_phy[0],&red);

        pg_type = red.m_RedComm.bcPageType;

        //read empty page,skip this block   
        if (PG_TYPE_FREE == pg_type)
        {
            break;
        }

        TimeStamp = red.m_RedComm.ulTimeStamp;

        if(TRUE == NFC_IsAddrUECC(&flash_phy[0]))
        {
            DBG_Printf("Skip ErrAddr Pu %d PhyBlk %d PhyPage %d TimeStamp %d\n",ucHalPhyPu,PhyBlk,PhyPage,TimeStamp);
            continue;
        }
        if(*pMaxTS < TimeStamp)
        {
            *pMaxTS = TimeStamp;
        }
        
    }
}

void  NFC_CalcTimeStamp(U32 ulSuperPU, U32 ulLUNInSuperPU, U32* pMaxTS)
{
    U16 PhyBlk;
    U16 PhyPage;

    U32 CalcMaxDataBlockTS = 0;
    U32 CalcMaxTableBlockTS = 0;
    FLASH_PHY flash_phy[PLN_PER_LUN];
    RED red;
    U8 pg_type;
    U32 TimeStamp;
    U16 BlkCnt = 0;
    U8 ucHalPhyPu; 

    ucHalPhyPu = HAL_NfcGetPhyPU(L2_GET_TLUN(ulSuperPU,ulLUNInSuperPU));

    for(PhyBlk = 0;PhyBlk < BLK_PER_PLN + RSV_BLK_PER_PLN;PhyBlk++)
    {       
        if (pPBIT[ulSuperPU]->m_PBIT_Entry[ulLUNInSuperPU][PhyBlk].bError)
            continue;

        BlkCnt++;

        //read table block,SLC mode
        if(TRUE == IsTableBlock(BlkCnt))
        {
            NFC_CalcMaxTableTS(ucHalPhyPu,PhyBlk,&CalcMaxTableBlockTS);
        }
        else if(TRUE == IsDataBlock(BlkCnt))
        {
            for(PhyPage = 0;PhyPage<PG_PER_BLK;PhyPage++)
            {
                NFC_SetFlashAddr(&flash_phy[0],ucHalPhyPu,PhyBlk,PhyPage);
                NFC_ReadRedData(&flash_phy[0], &red);

                pg_type = red.m_RedComm.bcPageType;

                if (0 == PhyPage && (BLOCK_TYPE_SEQ == red.m_RedComm.bcBlockType || BLOCK_TYPE_RAN == red.m_RedComm.bcBlockType))
                {
                    NFC_CalcSLCBlockTS(ucHalPhyPu,PhyBlk,&CalcMaxDataBlockTS);
                    break;
                }

                //read empty page,skip this block 
                if (PG_TYPE_FREE == pg_type)
                {                                   
                    break;
                }
                if(TRUE == NFC_IsAddrUECC(&flash_phy[0]))
                {
                    DBG_Printf("ErrAddr Pu %d PhyBlk %d PhyPage %d TimeStamp %d\n",L2_GET_TLUN(ulSuperPU,ulLUNInSuperPU),PhyBlk,PhyPage,TimeStamp);
                    continue;
                }

                if((pg_type == PAGE_TYPE_DATA) ||
                    (pg_type == PAGE_TYPE_RPMT && (PhyPage == PG_PER_BLK - 1)))
                {
                    TimeStamp = red.m_RedComm.ulTimeStamp;
                    if(CalcMaxDataBlockTS < TimeStamp)
                    {
                        CalcMaxDataBlockTS = TimeStamp;
                    }
                }
            }
        }
    }  
    *pMaxTS = max(CalcMaxDataBlockTS,CalcMaxTableBlockTS);
}
void Set_DBGPMTI_Info()
{
    U32 uPuNum;
    U32 PMTIndexInPu;
    U8 bInvalid8F = TRUE;
	
    for(uPuNum = 0; uPuNum < SUBSYSTEM_PU_NUM; uPuNum++)
    {
	    for(PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_PU;PMTIndexInPu++)
	    {
            dbg_g_PMTI[uPuNum]->m_PhyAddr[PMTIndexInPu][0].m_PPN = g_pNfcRebuildDbgInfo->m_pPMTAddr[uPuNum][PMTIndexInPu].m_PPN;
	    }
    }

    for (uPuNum = 0; uPuNum < SUBSYSTEM_PU_NUM; uPuNum++)
    {
        for (PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_PU; PMTIndexInPu++)
        {
            if (INVALID_8F != dbg_g_PMTI[uPuNum]->m_PhyAddr[PMTIndexInPu][0].m_PPN)
            {
                bInvalid8F = FALSE;
            }
        }
    }

    if (TRUE == bInvalid8F)
    {
        DBG_Printf("MCU %d: Set_DBGPMTI_Info error \n", HAL_GetMcuId());
        DBG_Getch();
    }
}
#include "L2_RT.h"
extern GLOBAL  RT *pRT;
void SIM_RebuildPMTManager_PMTI()
{
    U16 PhyBlk;
    U32 FreePageCnt = 0;
    U16 PhyPage;
    FLASH_PHY flash_phy[PLN_PER_LUN];
    RED red;
    U8 pg_type;
    U32 TimeStamp;
    U16 PPO;
    U32 BlkCnt;
    U32 PMTBlkSN;
    U32 PMTBlkCnt;
    U8 Pu;
    U32 uPMTIndexInPu;
    U8 ucSubSystemPu = 0;
    U8 ucHalPhyPu; 
    U32 bBadBlk;

    for(ucSubSystemPu = 0;ucSubSystemPu< SUBSYSTEM_PU_NUM;ucSubSystemPu++)
    {
        BlkCnt = 0;
        PMTBlkSN = 0;  
        PMTBlkCnt = 0;
        FreePageCnt = 0;
        Pu = ucSubSystemPu;
        ucHalPhyPu = HAL_NfcGetPhyPU(ucSubSystemPu);
        memset(dbg_p_m_PMTBlkManager[Pu],0,sizeof(PMTBlkManager));

        for(PhyBlk = 0;PhyBlk <= g_ulTableBlockEnd[Pu][0];PhyBlk++)
        {
            //1.skip bad block
            bBadBlk = L2_BbtIsGBbtBadBlock(Pu, PhyBlk);
            if(TRUE == bBadBlk)
            {
                continue;
            }
            BlkCnt++;
            if(BlkCnt <= (GLOBAL_BLOCK_COUNT + BBT_BLOCK_COUNT + RT_BLOCK_COUNT + AT0_BLOCK_COUNT))
            {
                continue;
            }
            PMTBlkCnt++;
            if(PMTBlkCnt > AT1_BLOCK_COUNT)
            {
                break;
            }
            FreePageCnt += PG_PER_SLC_BLK;
            PMTBlkSN = PMTBlkCnt - 1;

            //just for debug
            if(PhyBlk != pRT->m_RT[ucSubSystemPu].AT1BlockAddr[0][PMTBlkSN])
            {
                DBG_Printf("SN %d PhyBlk %d,AT1 %d\n",PMTBlkSN,PhyBlk,pRT->m_RT[ucSubSystemPu].AT1BlockAddr[0][PMTBlkCnt]);
                DBG_Getch();
            }

            for(PPO = 0; PPO < PG_PER_SLC_BLK; PPO++)
            {
#ifndef FLASH_INTEL_3DTLC
                PhyPage = HAL_FlashGetSLCPage(PPO) *PG_PER_WL;
#else
                PhyPage = HAL_FlashGetSLCPage(PPO);
#endif
                NFC_SetFlashAddr(&flash_phy[0],ucHalPhyPu,PhyBlk,PhyPage);
                NFC_ReadRedData(&flash_phy[0], &red);

                pg_type = red.m_RedComm.bcPageType;
                if (PG_TYPE_FREE == pg_type)
                {
                    if(PPO == 0)
                    {
                        dbg_p_m_PMTBlkManager[Pu]->m_PMTBlkInfo[PMTBlkSN].m_bFree = TRUE;
                    }
                    else
                    {
                        dbg_p_m_PMTBlkManager[Pu]->m_CurPPO = PPO;
                        dbg_p_m_PMTBlkManager[Pu]->m_CurBlkSN = PMTBlkSN;
                    }
                    break;
                }
                TimeStamp = red.m_RedComm.ulTimeStamp;
                if (PAGE_TYPE_PMT == pg_type && BLOCK_TYPE_PMT == red.m_RedComm.bcBlockType)
                {
                    FreePageCnt--;
                    if(PPO == 0)
                    {
                        dbg_p_m_PMTBlkManager[Pu]->m_PMTBlkInfo[PMTBlkSN].m_bFree = FALSE;
                    }
                    uPMTIndexInPu = red.m_PMTIndex;
                    if(red.m_RedComm.ulTimeStamp > g_pNfcRebuildDbgInfo->m_pPMTPageTS[ucSubSystemPu][uPMTIndexInPu] )
                    {
                        if(g_pNfcRebuildDbgInfo->m_pPMTPageTS[ucSubSystemPu][uPMTIndexInPu] > 0)
                        {
                            U32 OldBlkSN = g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu][uPMTIndexInPu].m_BlockInPU;
                            U8 OldCE = g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu][uPMTIndexInPu].m_PUSer;
                            dbg_p_m_PMTBlkManager[OldCE]->m_PMTBlkInfo[OldBlkSN].m_DirtyCnt++;
                        }
                        g_pNfcRebuildDbgInfo->m_pPMTPageTS[ucSubSystemPu][uPMTIndexInPu] = red.m_RedComm.ulTimeStamp;
                        g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu][uPMTIndexInPu].m_PUSer = Pu;
                        g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu][uPMTIndexInPu].m_OffsetInSuperPage = 0;
                        g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu][uPMTIndexInPu].m_BlockInPU = PMTBlkSN;
                        g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu][uPMTIndexInPu].m_PageInBlock = PPO;
                        g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu][uPMTIndexInPu].m_LPNInPage = 0;
                    }
                    else
                    {
                        dbg_p_m_PMTBlkManager[Pu]->m_PMTBlkInfo[PMTBlkSN].m_DirtyCnt++;
                    }
                }
            }
        }//end phyblk loop
        dbg_p_m_PMTBlkManager[Pu]->m_FreePagesCnt = FreePageCnt;

    }//end pu loop

    Set_DBGPMTI_Info();


    for(ucSubSystemPu = 0;ucSubSystemPu< SUBSYSTEM_PU_NUM;ucSubSystemPu++)
   {
	free(g_pNfcRebuildDbgInfo->m_pPMTPageTS[ucSubSystemPu] );
       g_pNfcRebuildDbgInfo->m_pPMTPageTS[ucSubSystemPu] = NULL;

       free(g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu] );
       g_pNfcRebuildDbgInfo->m_pPMTAddr[ucSubSystemPu] = NULL;
   }
	
}

void Set_DBG_PBIT_Info(U8 Pu, U16 PhyBlk, RED* pRed)
{
    dbg_pPBIT[Pu]->m_PBIT_Entry[0][PhyBlk].VirtualBlockAddr = pRed->m_RedComm.bsVirBlockAddr;
    dbg_pPBIT[Pu]->m_PBIT_Entry[0][PhyBlk].BlockType = pRed->m_RedComm.bcBlockType;
    dbg_pPBIT[Pu]->m_PBIT_Entry[0][PhyBlk].bAllocated = TRUE;
}
void Set_DBG_VBT_Info(U8 Pu,U16 PhyBlk, U16 VirBlk)
{
    dbg_pVBT[Pu]->m_VBT[VirBlk].PhysicalBlockAddr[0] = PhyBlk;
    dbg_pVBT[Pu]->m_VBT[VirBlk].bFree = FALSE;
    dbg_pVBT[Pu]->m_VBT[VirBlk].StripID = Pu;
    dbg_pVBT[Pu]->m_VBT[VirBlk].bPhyAddFindInFullRecovery = TRUE;
}

void SIM_RebuildSSD_Init()
{
    U32 i,j;

    g_pNfcRebuildDbgInfo = (NFC_REBUILD_DBG_INFO*)malloc(sizeof(NFC_REBUILD_DBG_INFO));
    memset(g_pNfcRebuildDbgInfo,0,sizeof(NFC_REBUILD_DBG_INFO));

    g_pNfcRebuildDbgInfo->m_pRedTS = (U32*)malloc(MAX_LPN_IN_DISK*sizeof(U32));
    memset(g_pNfcRebuildDbgInfo->m_pRedTS,0,MAX_LPN_IN_DISK*sizeof(U32));

    g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT = (PhysicalAddr*)malloc((MAX_LPN_IN_DISK+LPN_PER_BUF)*sizeof(U32));
    memset(g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT,0xFF,sizeof(PhysicalAddr)*(MAX_LPN_IN_DISK + LPN_PER_BUF)); 

    g_pNfcRebuildDbgInfo->m_pPendingLPN = (U32*)malloc(sizeof(U32)*MAX_PENDING_LPN);
    memset(g_pNfcRebuildDbgInfo->m_pPendingLPN,0xFF,sizeof(U32)*MAX_PENDING_LPN);

    for(i = 0;i< SUBSYSTEM_PU_NUM;i++)
    {
	    g_pNfcRebuildDbgInfo->m_pPMTPageTS[i] = (U32*)malloc(PMTPAGE_CNT_PER_PU*sizeof(U32));
	    memset(g_pNfcRebuildDbgInfo->m_pPMTPageTS[i],0,PMTPAGE_CNT_PER_PU*sizeof(U32));

	    g_pNfcRebuildDbgInfo->m_pPMTAddr[i] = (PhysicalAddr*)malloc(PMTPAGE_CNT_PER_PU*sizeof(U32));
	    memset(g_pNfcRebuildDbgInfo->m_pPMTAddr[i],0xFF,PMTPAGE_CNT_PER_PU*sizeof(U32));
    }

    for(i = 0;i< SUBSYSTEM_PU_NUM;i++)
    {
        g_pNfcRebuildDbgInfo->m_SameVirBlk[i] = INVALID_4F;

        for(j=0;j<TEMP_TARGET_CNT;j++)
        {
            g_pTempPuInfo[i][j] = (PuInfo*)malloc(sizeof(PuInfo));
            memset(g_pTempPuInfo[i][j],0xFF,sizeof(PuInfo));
        }
    }
}

U16 GetPPOFromPhyPage(U16 PhyPage)
{
    U16 PPO;

    for(PPO = 0; PPO < PG_PER_SLC_BLK; PPO++)
    {
#ifndef FLASH_INTEL_3DTLC
        if (PhyPage == HAL_FlashGetSLCPage(PPO) *PG_PER_WL)
#else
        if (PhyPage == HAL_FlashGetSLCPage(PPO))
#endif
        {
            return PPO;
        }
    }
    return INVALID_4F;
}

void Set_TargetBlk_Info(U8 Pu,U16 PhyBlk,U16 usPPO,RED* pRed)
{
    U8 ucTemp = 0;

    switch(pRed->m_RedComm.bcBlockType)
    {
    case BLOCK_TYPE_SEQ:
        if(g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] == INVALID_4F)
        {
            ucTemp = 0;
        }
        else
        {
            ucTemp = 1;
        }
        g_pTempPuInfo[Pu][ucTemp]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = usPPO;
        g_pTempPuInfo[Pu][ucTemp]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = pRed->m_RedComm.bsVirBlockAddr;
        //DBG_Printf("MCU %d Temp %d Pu %d SeqBlk 0x%x SeqPPO 0x%x\n",HAL_GetMcuId(),ucTemp,Pu,pRed->m_RedComm.bsVirBlockAddr,usPPO);
        break;
    case BLOCK_TYPE_RAN:
        if(g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC] == INVALID_4F)
        {
            ucTemp = 0;
        }
        else
        {
            ucTemp = 1;
        }
        g_pTempPuInfo[Pu][ucTemp]->m_TargetPPO[TARGET_HOST_GC] = usPPO;
        g_pTempPuInfo[Pu][ucTemp]->m_TargetBlk[TARGET_HOST_GC] = pRed->m_RedComm.bsVirBlockAddr;
        //DBG_Printf("MCU %d Temp %d Pu %d RndBlk 0x%x RndPPO 0x%x\n",HAL_GetMcuId(),ucTemp,Pu,pRed->m_RedComm.bsVirBlockAddr,usPPO);
        break;
#if 0
    case  BLOCK_TYPE_TLC_GC:
        if (g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_TLC_GC] == INVALID_4F)
        {
            ucTemp = 0;
        }
        else
        {
            ucTemp = 1;
        }
        g_pTempPuInfo[Pu][ucTemp]->m_TargetPPO[TARGET_TLC_GC] = usPPO;
        g_pTempPuInfo[Pu][ucTemp]->m_TargetBlk[TARGET_TLC_GC] = pRed->m_RedComm.bsVirBlockAddr;
        break;
    case  BLOCK_TYPE_TLC_SWL:
        if (g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_TLC_SWL] == INVALID_4F)
        {
            ucTemp = 0;
        }
        else
        {
            ucTemp = 1;
        }
        g_pTempPuInfo[Pu][ucTemp]->m_TargetPPO[TARGET_TLC_SWL] = usPPO;
        g_pTempPuInfo[Pu][ucTemp]->m_TargetBlk[TARGET_TLC_SWL] = pRed->m_RedComm.bsVirBlockAddr;
        break;    
#endif
    default:
        DBG_Printf("BlockType %d Error!\n",pRed->m_RedComm.bcBlockType);
        DBG_Getch();
        break;
    }
}
void Reset_DBGInfo()
{
    U8 Pu;
    U16 VirBlk;
    U8 i;

    for(Pu = 0;Pu< SUBSYSTEM_PU_NUM;Pu++)
    {

        memset(dbg_pPBIT[Pu],0xFF,sizeof(PBIT));
        memset(dbg_pVBT[Pu],0x0,sizeof(VBT));
        memset(dbg_g_PuInfo[Pu],0xFF,sizeof(PuInfo));

        for (i = 0; i < BLKTYPE_ALL; i++)
        {
            dbg_g_PuInfo[Pu]->m_DataBlockCnt[i] = 0;
            dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[i] = 0;
        }
        dbg_g_PuInfo[Pu]->m_TimeStamp = 0;

        for(VirBlk = 0; VirBlk < BLK_PER_PLN; VirBlk++)
        {
            dbg_pVBT[Pu]->m_VBT[VirBlk].StripID = Pu;
            dbg_pVBT[Pu]->m_VBT[VirBlk].Target = VBT_TARGET_INVALID;
        }
    }
}

void Set_NfcRebuildPMT(U8 Pu,U16 PhyBlk,U16 usPPO,RED* pRed)
{
    U32 ulLPN;
    U32 i;

    for(i=0;i<LPN_PER_BUF;i++)
    {
        ulLPN = pRed->m_DataRed.aCurrLPN[i];       
        if(INVALID_8F == ulLPN)
        {
            continue;
        }
        //FIRMWARE_LogInfo("Set_NfcRebuildPMT PU %d BLK 0x%x PG 0x%x LPNOffset %d LPN 0x%x\n", Pu, PhyBlk, usPPO, i, ulLPN);
        if(pRed->m_RedComm.ulTimeStamp > g_pNfcRebuildDbgInfo->m_pRedTS[ulLPN])//2.update DBG_PMT
        {
            g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_PUSer = Pu;
            g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_BlockInPU = pRed->m_RedComm.bsVirBlockAddr;
            g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_PageInBlock = usPPO;
            g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_LPNInPage = i;
            g_pNfcRebuildDbgInfo->m_pRedTS[ulLPN] = pRed->m_RedComm.ulTimeStamp;
        } 
    }
}

void Adjust_DBGPMT(U32 ulLPNCnt)
{
#if 1
    #ifdef DBG_PMT
    U32 ulLPN;
    for(ulLPN=0; ulLPN < MAX_LPN_IN_DISK ;ulLPN++)
    {
        if(g_DebugPMT[ulLPN].m_PPN == INVALID_8F)
            continue;

        if(g_DebugPMT[ulLPN].m_PPN != g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_PPN)
        {
            //DBG_Printf("LPN 0x%x NFC PMT 0x%x Dram PMT 0x%x\n",ulLPN,g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_PPN,g_DebugPMT[ulLPN].m_PPN);
            g_DebugPMT[ulLPN].m_PPN = INVALID_8F;
        }        
    }
    #endif
#else
    U32 i;
    for(i=0; i < ulLPNCnt ;i++)
    {
        ulLPN = g_pNfcRebuildDbgInfo->m_pPendingLPN[i];

        if(ulLPN >= MAX_LPN_IN_DISK)
            continue;

        if(g_DebugPMT[ulLPN].m_PPN == INVALID_8F)
            continue;
       
        if(g_DebugPMT[ulLPN].m_PPN != g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_PPN)
        {
            DBG_Printf("LPN 0x%x NFC PMT 0x%x Dram PMT 0x%x\n",ulLPN,g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_PPN,g_DebugPMT[ulLPN].m_PPN);
            g_DebugPMT[ulLPN].m_PPN = g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT[ulLPN].m_PPN;
        }        
    }
#endif
}

void Set_DbgPuInfo(U8 Pu,U32* pFreePageCnt)
{
    U16 VirBlk;
    U8 ucTemp;
    U8 ucAnotherTemp;
    BOOL bSet = FALSE;

    U32 ulSameVirBlk = g_pNfcRebuildDbgInfo->m_SameVirBlk[Pu];
    if(ulSameVirBlk == INVALID_4F)
    {
        dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC];
        dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC];

        dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
        dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];

        dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_WRITE] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_TLC_WRITE];
        dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_TLC_WRITE] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_TLC_WRITE];
#if 0
        dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_GC] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_TLC_GC];
        dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_TLC_GC] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_TLC_GC];

        dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_SWL] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_TLC_SWL];
        dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_TLC_SWL] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_TLC_SWL];
#endif
    }
    else
    {
        for(ucTemp = 0; ucTemp < TEMP_TARGET_CNT; ucTemp++)
        {
            ucAnotherTemp = TEMP_TARGET_CNT - 1 - ucTemp;

            //SameVirBlk == RndBlk[Pu][0] == RndBlk[Pu][1],Need to compare RndPPO, caused by table rebuild move target block
            if(ulSameVirBlk == g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC]
                && ulSameVirBlk == g_pTempPuInfo[Pu][1]->m_TargetBlk[TARGET_HOST_GC])
            {
                DBG_Printf("Pu %d SameVirBlk %d Temp0 RndBlk %d RndPPO %d,Temp1 RndBlk %d RndPPO %d\n",Pu,ulSameVirBlk,
                    g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC],g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC],
                    g_pTempPuInfo[Pu][1]->m_TargetBlk[TARGET_HOST_GC],g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_GC]);

                //Set RndBlock
                if((g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_GC] >= g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC]) &&
                    (g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_GC] < PG_PER_SLC_BLK))
                {
                    *pFreePageCnt +=  g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC];
                    dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_GC];
                    dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] = g_pTempPuInfo[Pu][1]->m_TargetBlk[TARGET_HOST_GC];
                }
                else
                {
                    *pFreePageCnt += g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_GC];
                    dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC];
                    dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC];
                }
                dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_SLC]--;

                //Set SeqBlock
                dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];

                //Set SLCBlock
                bSet = TRUE;
                break;
            }
            //SameVirBlk == SeqBlk[Pu][0] == SeqBlk[Pu][1],Need to compare SeqPPO, caused by table rebuild move target block
            else if(ulSameVirBlk == g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML]
                && ulSameVirBlk == g_pTempPuInfo[Pu][1]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
            {
                DBG_Printf("Pu %d SameVirBlk %d Temp0 SeqBlk %d SeqPPO %d,Temp1 SeqBlk %d SeqPPO %d\n",Pu,ulSameVirBlk,
                    g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML],g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML],
                    g_pTempPuInfo[Pu][1]->m_TargetBlk[TARGET_HOST_WRITE_NORAML],g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);

                //Set SeqTarget
                if((g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] >= g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]) &&
                    (g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] < PG_PER_SLC_BLK))
                {
                    *pFreePageCnt +=  g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                    dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                    dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][1]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
                }
                else
                {
                    *pFreePageCnt += g_pTempPuInfo[Pu][1]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                    dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                    dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
                }
                dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_SLC]--;

                //Set RndTarget
                dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC];
                dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC];

                //Set SLCBlock
                bSet = TRUE;
                break;
            }
            //SameVirBlk == SeqBlock, caused by StaticWL move block
            else if(ulSameVirBlk == g_pTempPuInfo[Pu][ucTemp]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
            {
                *pFreePageCnt +=  g_pTempPuInfo[Pu][ucTemp]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];

                dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][ucAnotherTemp]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][ucAnotherTemp]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];

                dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC];
                dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC];

                bSet = TRUE;
                break;
            }
            //SameVirBlk == RndBlk, caused by StaticWL move block
            else if(ulSameVirBlk == g_pTempPuInfo[Pu][ucTemp]->m_TargetBlk[TARGET_HOST_GC])
            {
                *pFreePageCnt += g_pTempPuInfo[Pu][ucTemp]->m_TargetPPO[TARGET_HOST_GC];

                dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = g_pTempPuInfo[Pu][ucAnotherTemp]->m_TargetPPO[TARGET_HOST_GC];
                dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] = g_pTempPuInfo[Pu][ucAnotherTemp]->m_TargetBlk[TARGET_HOST_GC];

                dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];

                //Set SLCBlock
                bSet = TRUE;
                break;
            }
        }//end for loop

        //Static WL move block,SrcBlk and DstBlk are both full data block
        if(FALSE == bSet)
        {
            dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_GC];
            dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_GC];

            dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
            dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = g_pTempPuInfo[Pu][0]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
        }
    }

    //DBG_Printf("Pu %d SeqBlk 0x%x SeqPPO 0x%x RndBlk 0x%x RndPPO 0x%x\n",Pu,dbg_g_PuInfo[Pu]->m_SeqBlock,
    //    dbg_g_PuInfo[Pu]->m_SeqPPO, dbg_g_PuInfo[Pu]->m_RndBlock,dbg_g_PuInfo[Pu]->m_RndPPO);

    VirBlk = dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC];
    dbg_pVBT[Pu]->m_VBT[VirBlk].Target = VBT_TARGET_HOST_GC;

    VirBlk = dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
    dbg_pVBT[Pu]->m_VBT[VirBlk].Target = VBT_TARGET_HOST_W;
}

void Set_DbgPBIT_WithPuInfo(U8 Pu)
{
    U8 i;

    for (i = 0; i < BLKTYPE_ALL; i++)
    {
        //dbg_pPBIT[Pu]->m_AllocatedDataBlockCnt[i] = dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[i];
        dbg_pPBIT[Pu]->m_TotalDataBlockCnt[i] = dbg_g_PuInfo[Pu]->m_DataBlockCnt[i];
    }
    dbg_pPBIT[Pu]->m_TargetBlock[TARGET_HOST_WRITE_NORAML] = dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
    dbg_pPBIT[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
    dbg_pPBIT[Pu]->m_TargetBlock[TARGET_HOST_GC] = dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC];
    dbg_pPBIT[Pu]->m_TargetPPO[TARGET_HOST_GC] = dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC];
#if 0
    dbg_pPBIT[Pu]->m_TargetBlock[TARGET_TLC_GC] = dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_TLC_GC];
    dbg_pPBIT[Pu]->m_TargetPPO[TARGET_TLC_GC] = dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_GC];
    dbg_pPBIT[Pu]->m_TargetBlock[TARGET_TLC_SWL] = dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_TLC_SWL];
    dbg_pPBIT[Pu]->m_TargetPPO[TARGET_TLC_SWL] = dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_SWL];
#endif
}

void SIM_Rebuild_SLCBlk_Info(U8 ucSubSystemPu,U16 PhyBlk,U32* pFreePageCnt, U32* pTS)
{
    U8 Pu = ucSubSystemPu;
    U8 ucHalPhyPu = HAL_NfcGetPhyPU(ucSubSystemPu);
    U16 usPPO;
    U16 PhyPage;
    U8 pg_type;
    FLASH_PHY flash_phy[PLN_PER_LUN];
    RED red;
    RED red2;

    *pFreePageCnt += PG_PER_SLC_BLK;

    for(usPPO = 0; usPPO < PG_PER_SLC_BLK; usPPO++)
    {
#ifndef FLASH_INTEL_3DTLC
        PhyPage = HAL_FlashGetSLCPage(usPPO)*PG_PER_WL;
#else
        PhyPage = HAL_FlashGetSLCPage(usPPO);
#endif
        NFC_SetFlashAddr(&flash_phy[0],ucHalPhyPu,PhyBlk,PhyPage);
        NFC_ReadRedData(&flash_phy[0], &red);

        pg_type = red.m_RedComm.bcPageType;
        if (PG_TYPE_FREE == pg_type)
        {
            //read pre page
#ifndef FLASH_INTEL_3DTLC
            PhyPage = HAL_FlashGetSLCPage(usPPO - 1)*PG_PER_WL;
#else
            PhyPage = HAL_FlashGetSLCPage(usPPO -1);
#endif
            NFC_SetFlashAddr(&flash_phy[0],ucHalPhyPu,PhyBlk,PhyPage);
            NFC_ReadRedData(&flash_phy[0], &red2);

            Set_TargetBlk_Info(Pu,PhyBlk,usPPO,&red2);
            break;
        }
        (*pFreePageCnt)--;

        if(0 == usPPO)
        {
            Set_DBG_PBIT_Info(Pu,PhyBlk,&red);
            Set_DBG_VBT_Info(Pu,PhyBlk,red.m_RedComm.bsVirBlockAddr);
            dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_SLC]++;
        }

        //update NFC rebuild PMT
        Set_NfcRebuildPMT(Pu,PhyBlk,usPPO,&red);

        //update MaxTS 
        if(TRUE == NFC_IsAddrUECC(&flash_phy[0]))
        {
            DBG_Printf("Skip ErrAddr Pu %d PhyBlk %d PhyPage %d TimeStamp %d\n",Pu,PhyBlk,PhyPage,red.m_RedComm.ulTimeStamp);
            continue;
        }                  
        if((*pTS) < red.m_RedComm.ulTimeStamp)
        {
            *pTS = red.m_RedComm.ulTimeStamp;
        }
    }
    return;
}

void  SIM_Rebuild_DBGInfo()
{
    U8 Pu;
    U16 PhyBlk,VirBlk;
    U16 PhyPage;
    FLASH_PHY flash_phy[PLN_PER_LUN];
    RED red,last_red;
    U8 pg_type;
    U32 i,j;
    U32 CalcMaxDataBlockTS = 0;
    U32 CalcMaxTableBlockTS = 0;
    U32 FreePageCnt = 0;
    U32 ulLPNCnt1 = 0,ulLPNCnt2 = 0;   
    U32 BlkCnt;
    U8 ucSubSystemPu = 0;
    U8 ucHalPhyPu; 
    U32 ulSLCBlkCnt = 0;
    U16 PPO;
    U32 SLCBlkend,TLCBlkEnd;
    BOOL bSLCBlkFlag;
    U32 uBlkPageCnt = PG_PER_BLK;

    Reset_DBGInfo();

    for(ucSubSystemPu = 0;ucSubSystemPu< SUBSYSTEM_PU_NUM;ucSubSystemPu++)
    {
        U32 bBadBlk;
        BlkCnt = 0;
        FreePageCnt = 0;
        ulSLCBlkCnt = 0;
        CalcMaxTableBlockTS = 0;
        CalcMaxDataBlockTS = 0;
        Pu = ucSubSystemPu;
        ucHalPhyPu = HAL_NfcGetPhyPU(ucSubSystemPu);
        //Get SLCBlkEnd TLCBlkEnd
        L2_BbtGetSlcTlcBlock(ucSubSystemPu, &SLCBlkend, &TLCBlkEnd);

        for(PhyBlk = 0;PhyBlk < (BLK_PER_PLN + RSV_BLK_PER_PLN);PhyBlk++)
        {       
            dbg_pPBIT[Pu]->m_PBIT_Entry[0][PhyBlk].VirtualBlockAddr = INVALID_4F;

            //1.skip bad block
            bBadBlk = L2_BbtIsGBbtBadBlock(Pu, PhyBlk);
            if(TRUE == bBadBlk)
            {
                continue;
            }
            BlkCnt++;
            //2.Calc table block max timestamp
            if(TRUE == IsTableBlock(BlkCnt))
            {
                NFC_CalcMaxTableTS(ucHalPhyPu,PhyBlk,&CalcMaxTableBlockTS);
            }
            else if(TRUE == IsDataBlock(BlkCnt))
            {
                if (PhyBlk <= SLCBlkend )
                {
                    bSLCBlkFlag = TRUE;
                    dbg_g_PuInfo[Pu]->m_DataBlockCnt[BLKTYPE_SLC]++;
                }
                else if (PhyBlk >= TLCBlkEnd )
                {
                    bSLCBlkFlag = FALSE;
                    uBlkPageCnt = PG_PER_BLK;
                    dbg_g_PuInfo[Pu]->m_DataBlockCnt[BLKTYPE_TLC]++;
                    //FreePageCnt += PG_PER_BLK;
                }
                else
                {
                    bSLCBlkFlag = FALSE;
                    dbg_pPBIT[Pu]->m_PBIT_Entry[0][PhyBlk].bBackup = TRUE;
                    continue;
                }

                //TLC last page empty , ignore this block
                if(FALSE== bSLCBlkFlag)
                {                
                    NFC_SetFlashAddr(&flash_phy[0], ucHalPhyPu, PhyBlk, uBlkPageCnt-1);
                    NFC_ReadRedData(&flash_phy[0], &last_red);
                    
                    pg_type = last_red.m_RedComm.bcPageType;
                    if(PG_TYPE_FREE == pg_type)
                        continue;                
                }

                for(PhyPage = 0;PhyPage < uBlkPageCnt;PhyPage++)
                {
                    PPO = GetPPOFromPhyPage(PhyPage);

                    NFC_SetFlashAddr(&flash_phy[0], ucHalPhyPu, PhyBlk, PhyPage);
                    NFC_ReadRedData(&flash_phy[0], &red);

                    pg_type = red.m_RedComm.bcPageType;

                    if(0 == PhyPage && INVALID_4F == PPO)
                    {
                        DBG_Getch();
                    }

                    //1.Skip EmptyBlock
                    if(0 == PhyPage && PG_TYPE_FREE == pg_type)
                    {
                        if( bSLCBlkFlag)
                        {
                            FreePageCnt += PG_PER_SLC_BLK;
                        }
                        break;
                    }
                    
                    //SLC Block
                    if(bSLCBlkFlag)
                    {
                        ulSLCBlkCnt++;
                        SIM_Rebuild_SLCBlk_Info(ucSubSystemPu,PhyBlk,&FreePageCnt,&CalcMaxDataBlockTS);
                        break;
                    }

                    //TLC Block

                    //FreePageCnt--;
                    if (pg_type == PAGE_TYPE_RPMT && (PhyPage == PG_PER_BLK - 1)) //update VBT
                    {
                        VirBlk = red.m_RedComm.bsVirBlockAddr; 
                        dbg_pVBT[Pu]->m_VBT[VirBlk].Target = VBT_NOT_TARGET;
                    }                
                    //ingore tlc last page err case
                    if(PhyPage == PG_PER_BLK - 1 )
                    {            
                        //same virblk with different phyblk   
                        VirBlk = red.m_RedComm.bsVirBlockAddr;
                        if(dbg_pVBT[Pu]->m_VBT[VirBlk].bPhyAddFindInFullRecovery == TRUE) 
                        {      
                            /* found two TLC phyblk has same VirBlk, caused by SWL, just skip it*/
                            DBG_Printf("Pu %d PhyBlk 0x%x VirBlk 0x%x PrePhyBlk 0x%x\n", Pu, PhyBlk, VirBlk, dbg_pVBT[Pu]->m_VBT[VirBlk].PhysicalBlockAddr[0]);
                            continue;
                        }
                        Set_DBG_PBIT_Info(Pu,PhyBlk,&red);
                        Set_DBG_VBT_Info(Pu,PhyBlk,red.m_RedComm.bsVirBlockAddr);
                        dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_TLC]++;
                        //FIRMWARE_LogInfo("Pu %d VirBlk %d PhyBlk %d TLCAllocateBlkCnt %d\n", Pu, red.m_RedComm.bsVirBlockAddr, PhyBlk, dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_TLC]);
                    }

                    if(TRUE == NFC_IsAddrUECC(&flash_phy[0]))
                    {
                        DBG_Printf("Skip ErrAddr Pu %d PhyBlk %d PhyPage %d TimeStamp %d\n",Pu,PhyBlk,PhyPage,red.m_RedComm.ulTimeStamp);
                        continue;
                    }
                    
                    Set_NfcRebuildPMT(Pu,PhyBlk,PhyPage,&red);//update NFC rebuild PMT

                    if(CalcMaxDataBlockTS < red.m_RedComm.ulTimeStamp)
                    {
                        CalcMaxDataBlockTS = red.m_RedComm.ulTimeStamp;
                    }
                }//end phypg loop
            }
        }//end phyblk loop

        //FreePageCnt -= ulSLCBlkCnt*PG_PER_SLC_BLK;

        Set_DbgPuInfo(Pu,&FreePageCnt);

        dbg_g_PuInfo[Pu]->m_SLCMLCFreePageCnt = FreePageCnt;

        dbg_g_PuInfo[Pu]->m_TimeStamp = max(CalcMaxDataBlockTS,CalcMaxTableBlockTS);


        for (i = 0; i < BLKTYPE_ALL; i++)
        {
            dbg_pPBIT[Pu]->m_AllocatedDataBlockCnt[i] = dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[i];
        }

         if(dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] == INVALID_4F && dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == INVALID_4F)
        {
            dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = 0;
            dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_SLC]++;
        }

        if(dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_HOST_GC] == INVALID_4F && dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] == INVALID_4F)
        {
            dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_HOST_GC] = 0;
            dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_SLC]++;
        }

        //Special process
         if(dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_TLC_WRITE] == INVALID_4F && dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_WRITE] == INVALID_4F)
        {
            dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_WRITE] = 0;
            //dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_TLC]++;
        }

#if 0
         if(dbg_g_PuInfo[Pu]->m_TargetBlk[TARGET_TLC_GC] == INVALID_4F && dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_GC] == INVALID_4F)
        {
            dbg_g_PuInfo[Pu]->m_TargetPPO[TARGET_TLC_GC] = 0;
            dbg_g_PuInfo[Pu]->m_AllocateBlockCnt[BLKTYPE_SLC]++;
        }
#endif

        Set_DbgPBIT_WithPuInfo(Pu); //update PBIT using PuInfo

        for(j=0;j<TEMP_TARGET_CNT;j++)
        {
            free(g_pTempPuInfo[Pu][j]);
            g_pTempPuInfo[Pu][j] = NULL;
        }
        

    }//end pu loop

    L3_GetPendingLPN(&g_pNfcRebuildDbgInfo->m_pPendingLPN[0],&ulLPNCnt1);
    NFC_GetPendingLPN(&g_pNfcRebuildDbgInfo->m_pPendingLPN[ulLPNCnt1],&ulLPNCnt2);

    //L2_CheckPMTI();
    Adjust_DBGPMT(ulLPNCnt1 + ulLPNCnt2);

    free(g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT);
    g_pNfcRebuildDbgInfo->m_pNfcRebuildPMT = NULL;
    free(g_pNfcRebuildDbgInfo->m_pRedTS);
    g_pNfcRebuildDbgInfo->m_pRedTS = NULL;
    free(g_pNfcRebuildDbgInfo->m_pPendingLPN);
    g_pNfcRebuildDbgInfo->m_pPendingLPN = NULL;
    free(g_pNfcRebuildDbgInfo); 
    g_pNfcRebuildDbgInfo = NULL;

}


// Wait L3 Empty, will call it
void SIM_BackTableFromDram(void)
{
    Dbg_Record_TableRebuild_Info();
}

// NotWaitL3 Empty, will call it
void SIM_BackTalbeFromFlash(void)
{
    U8 ucPhyPuInCh;
    U8 ucLunInPhyPu;
    U8 ucCh;
    NFCM_LUN_LOCATION tNfcOrgStruct = { 0 };
    //if(g_bInjectError)
    //{
    //    L2_CheckPMT();
    //    L2_RecordPMT();
    //}

    SIM_RebuildSSD_Init();
    SIM_RebuildPMTManager_PMTI();
    SIM_Rebuild_DBGInfo();

    DBG_Printf("MCU%d Reduild SSD done.\n",HAL_GetMcuId());

#if 0
    for(i=0; i<SUBSYSTEM_PU_NUM; i++)
    {
        ucHalPhyPu = HAL_NfcGetPhyPU(i);

        NFC_InterfaceResetCQ(ucHalPhyPu,0);
        NFC_InterfaceResetCQ(ucHalPhyPu,1);
    }
#else
    for (ucPhyPuInCh = 0; ucPhyPuInCh < NFC_PU_PER_CH; ++ucPhyPuInCh)
    {
        for (ucLunInPhyPu = 0; ucLunInPhyPu < NFC_LUN_MAX_PER_PU; ++ucLunInPhyPu)
        {
            for (ucCh = 0; ucCh < NFC_CH_TOTAL; ++ucCh)
            {
                tNfcOrgStruct.ucCh = ucCh;
                tNfcOrgStruct.ucPhyPuInCh = ucPhyPuInCh;
                tNfcOrgStruct.ucLunInPhyPu = ucLunInPhyPu;

                NFC_InterfaceResetCQ(&tNfcOrgStruct, 0);
                NFC_InterfaceResetCQ(&tNfcOrgStruct, 1);
            }
        }
    }
#endif
}

void SIM_BakupTableRebuildInfo(U8 uTableRebuildSource)
{
    
    if (TABLE_REBUILD_DRAM == uTableRebuildSource)
    {
        //FIRMWARE_LogInfo("============SIM_BackTalbeFromDram===========\n");
        SIM_BackTableFromDram();
    }

    if (TABLE_REBUILD_FLASH == uTableRebuildSource)
    {
        //FIRMWARE_LogInfo("============SIM_BackTalbeFromFlash===========\n");
        SIM_BackTalbeFromFlash();
    }

    return;
}

#endif // end #ifdef SIM