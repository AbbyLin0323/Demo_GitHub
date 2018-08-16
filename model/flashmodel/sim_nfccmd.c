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

Filename     : sim_nfccmd.c
Version       : Ver 1.0
Date           :
Author        : Peterlgchen

Description:

Modification History:
20090522    peterlgchen 01 first create
20120214    PengfeiYu   002 modified
20120409    GavinYin    003 modified
*************************************************/
#include <stdio.h>
#include <stddef.h>
#include "BaseDef.h"
#include "sim_flash_config.h"
#include "sim_NormalDSG.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_MemoryMap.h"
#include "Disk_Config.h"
#include "memory_access.h"
#include "sim_flash_common.h"
#include "sim_SGE.h"
#include "sim_flash_interface.h"
#include "sim_nfccmd.h"
#include "model_config.h"
#include "action_define.h"
#include "Sim_XOR_Interface.h"
#include "HAL_DecStsReport.h"
#ifndef L1_FAKE
#include "L2_TableBlock.h"
#endif
#include "checklist_parse.h"
#include "sim_flash_status.h"
#include "Sim_DEC_Status.h"
#include "flash_meminterface.h"

#ifdef SIM_XTENSA
    #include "hfmid.h"
#endif

//tobey start
NORMAL_DSG_ENTRY NfcDsg;
NORMAL_DSG_ENTRY * l_pNfcDsg;
U16 l_usCurDsgID;
U16 l_usNextDsgID;
U8 l_ucNcqMode;
//tobey end

//extern ST_CE_ERROR_ENTRY   pu_err[NFC_CH_TOTAL][NFC_PU_PER_CH];

FLASH_PHY g_LastHandleAddr[NFC_MODEL_LUN_SUM];
U32 g_ulBufAddr;
extern ERR_INJ_PARRM CRCErrParamSlot[ERR_INJ_SLOT_MAX];

extern volatile BOOL g_bReSetFlag;
extern BOOL IsDuringTableRebuild();
extern BOOL IsDuringRebuildDirtyCnt(U32 ulMcuId);
extern CRITICAL_SECTION g_CHCriticalSection[];
extern U8 NF_GetCmdType(U8 ucCmdCode);
extern U8 NFC_GetPartInWL(U8 ucCmdCode);
extern UINT32 Flash_Talbe_GetPageType(PFLASH_PHY pFlash_phy);

extern BOOL g_bWearLevelingStatistic;

#ifdef DBG_TABLE_REBUILD
extern U32 Host_GetDataCntHighBit(U32 lba);
extern void Host_ClearDataCntHighBit(U32 lba,U32 ulWriteCnt);
extern U32 Host_GetDataCnt(U32 lba);
extern BOOL Host_IsHitHCmdTable(U32 ulSystemLba);
extern void L2_AddErrAddrOfCheckPMT(U8 PUSer, U16 VirBlk, U16 Pg);
extern void L2_RemoveErrAddrOfCheckPMT(U8 PUSer, U16 VirBlk);
extern U16 GetPPOFromPhyPage(U16 PhyPage);
#endif

U8 g_LPNInRedOffSet = offsetof(SIM_NFC_RED, m_DataRed);

extern BOOL g_bSquashMode;

/*==============================================================================
Func Name  : NFC_CheckReadErrOccur
Input      : U8 ucppu
             U8 pg_type
             ST_FLASH_CMD_PARAM *pFlashCmdParam
             ST_FLASH_READ_RETRY_PARAM *p_flash_readrety_param
Output     : None
Return Val : TRUE:Need report the error
             FALSE:don't need to reprot the error
Discription: Check Err Injection Status, to skip set buf map or add chain number.
Usage      :
History    :
    1. 2014.10.4 JasonGuo create function
    2. 2015.01.16 NinaYang Modify
       check whether read error occur:
        if occur, record err_type and err_flag,and check the error whether need to report.
        if not,return false.
==============================================================================*/
U8 NFC_ErrInj(U8 err_type,  ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 ucNfcErrCode = err_type;

    if ((NF_ERR_TYPE_UECC == ucNfcErrCode) && (TRUE == pFlashCmdParam->bsBypassRdErr || TRUE == pFlashCmdParam->bsRawReadEn))
    {
        pFlashCmdParam->module_inj_en = FALSE;
        pFlashCmdParam->module_inj_type = NF_SUCCESS;
        ucNfcErrCode = NF_SUCCESS;
    }
    else
    {
        pFlashCmdParam->module_inj_en = TRUE;
        pFlashCmdParam->module_inj_type = ucNfcErrCode;
    }

    return ucNfcErrCode;
}

U32 L1_CalcLPNForMoudle(U32 phyLPN)
{
#ifdef LPN_TO_CE_REMAP
    U8 targetPU;
    U32 pageNum;
    U32 lpnOffset;

    pageNum = phyLPN>>LPN_PER_BUF_BITS;
    lpnOffset = phyLPN & LPN_PER_BUF_MSK;

    targetPU =( ( pageNum )^
                ( pageNum/CE_SUM )^
                ( pageNum/CE_SUM/CE_SUM )^
                ( pageNum/CE_SUM/CE_SUM/CE_SUM )^
                ( pageNum/CE_SUM/CE_SUM/CE_SUM/CE_SUM ) )%CE_SUM;

    pageNum = ( pageNum & ~(CE_SUM-1) ) + targetPU;

    return ( (pageNum<<LPN_PER_BUF_BITS) + lpnOffset );
#else
    return phyLPN;
#endif
}

void NFC_GetSparePhyAddr(U32* pRedData, ST_FLASH_ADDR *pFlashAddr, U8 nLPNInPage)
{
#ifdef DBG_PMT_SPARE
    pFlashAddr->pu = ((SpareArea *)pRedData)->m_PMTDebug[nLPNInPage].m_PUSer;
    pFlashAddr->pbn = ((SpareArea *)pRedData)->m_PMTDebug[nLPNInPage].m_BlockInPU;
    pFlashAddr->ppo = ((SpareArea *)pRedData)->m_PMTDebug[nLPNInPage].m_PageInBlock;

    pFlashAddr->pu = ((SIM_NFC_RED *)pRedData)->m_PMTDebug[nLPNInPage].m_PUSer;
    pFlashAddr->pbn = ((SIM_NFC_RED *)pRedData)->m_PMTDebug[nLPNInPage].m_BlockInPU;
    pFlashAddr->ppo = ((SIM_NFC_RED *)pRedData)->m_PMTDebug[nLPNInPage].m_PageInBlock;
#endif
}

void initDmaAddr(NFCQ_ENTRY *p_nf_cq_entry)
{
    g_remainSecToXfer = p_nf_cq_entry->bsDmaTotalLength;
    g_finishSecInDmaEntry = 0;

    l_usCurDsgID = 0;
    l_usNextDsgID = 0;
    memset(&NfcDsg,0,sizeof(NORMAL_DSG_ENTRY));
    l_pNfcDsg = NULL;
}

/*
    input: pFlashCmdParam
    output: pointer of next DMA target address. when return value is TRUE, which means should update buff map after
            data transfer finishing, pointer to the buff map bit also passed to caller
*/
BOOL NFC_GetDMATargetAddr(ST_FLASH_CMD_PARAM* pFlashCmdParam, U32 **pAddr, U8 *pBmBitPos, U32 *Bmid, BOOL *pNcq1stEn, U16 *pReleaseDsgFlag)
{
    U32 curDmaAddr;
    BOOL bUpdateBm;
    BOOL bBmEn;
    U8 ucDMATotalLength;
    U8 ucSecLenInFirst4K;

    bUpdateBm = FALSE;
    ucDMATotalLength = pFlashCmdParam->p_nf_cq_entry->bsDmaTotalLength;

    if(NULL == l_pNfcDsg)
    {
        l_ucNcqMode = pFlashCmdParam->ncq_mode;
        l_usCurDsgID = pFlashCmdParam->p_nf_cq_entry->bsFstDsgPtr;

        DSG_FetchNormalDsg(l_usCurDsgID, &NfcDsg);
        l_pNfcDsg = &NfcDsg;
        g_bmBitPos = l_pNfcDsg->bsMapSecOff >> SEC_PER_LPN_BITS;
        l_usNextDsgID = l_pNfcDsg->bsNextDsgId;
    }
    else
    {
        if(0 == g_finishSecInDmaEntry)
        {
            if(1 != l_pNfcDsg->bsLast)
            {
                DSG_FetchNormalDsg(l_usCurDsgID, &NfcDsg);
                l_pNfcDsg = &NfcDsg;
                g_bmBitPos = l_pNfcDsg->bsMapSecOff >> SEC_PER_LPN_BITS;
                l_usNextDsgID = l_pNfcDsg->bsNextDsgId;
            }
            else
            {
                return bUpdateBm;
            }
        }
    }

    // for soft dec cal redundant addr.
    if (TRUE == pFlashCmdParam->p_nf_cq_entry->bsRawReadEn)
    {
        g_ulBufAddr = l_pNfcDsg->bsDramAddr << 1;
    }

    curDmaAddr = (l_pNfcDsg->bsDramAddr << 1) + g_finishSecInDmaEntry * SEC_SIZE;
#if 0
    if ((pFlashCmdParam->bsIsTlcMode == TRUE) && (pFlashCmdParam->bsIs6DsgIssue == FALSE) &&
        pFlashCmdParam->bsCmdType == CMD_CODE_PROGRAM)
    {
        if (1 == pFlashCmdParam->bsPlnNum)
        {
            curDmaAddr = (l_pNfcDsg->bsDramAddr << 1) + g_finishSecInDmaEntry * SEC_SIZE + (pFlashCmdParam->bsPageInWL * LOGIC_PG_SZ);
        }
        else
        {
            curDmaAddr = (l_pNfcDsg->bsDramAddr << 1) + g_finishSecInDmaEntry * SEC_SIZE + (pFlashCmdParam->bsPageInWL * LOGIC_PIPE_PG_SZ);
        }
    }
    else
    {
        curDmaAddr = (l_pNfcDsg->bsDramAddr << 1) + g_finishSecInDmaEntry * SEC_SIZE;
    }
#endif

    bBmEn = pFlashCmdParam->p_nf_cq_entry->bsBmEn;
    *Bmid = l_pNfcDsg->bsBuffMapId;
    g_finishSecInDmaEntry++;

    //when 4k finish, current is the last sector in current DSG or last DSG, we need to update buffmap
    if(((0 == (l_pNfcDsg->bsMapSecOff + g_finishSecInDmaEntry) % 8) ||
        (1 == g_remainSecToXfer) || ((g_finishSecInDmaEntry * SEC_SIZE) == (l_pNfcDsg->bsXferByteLen)))
        && (TRUE == bBmEn))
    {
        bUpdateBm = TRUE;
        *pBmBitPos = g_bmBitPos;
        g_bmBitPos++;
    }

    //move forwared when current DSG finished
    if ((g_finishSecInDmaEntry  * SEC_SIZE) == l_pNfcDsg->bsXferByteLen)
    {
        g_finishSecInDmaEntry = 0;
        *pReleaseDsgFlag = (1 << 15) | (l_usCurDsgID);// bit15 = 1, need to release DSG; bit[14:0] = DSG id to release
        l_usCurDsgID = l_usNextDsgID;
    }

    g_remainSecToXfer--;
    *pAddr = (U32*)curDmaAddr;

    //when 4k finished or current is the last sector in last DSG report ncq_1st
    if (TRUE == pFlashCmdParam->ncq_1st_en)
    {
        if (NCQMD_4KB_FINISH == l_ucNcqMode)
        {
            if ((pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecStart >> SEC_PER_LPN_BITS)
              == ((pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecStart
                + pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecLength - 1) >> SEC_PER_LPN_BITS))
            {
                ucSecLenInFirst4K = (U8)pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecLength;
            }
            else
            {
                ucSecLenInFirst4K = SEC_PER_LPN - (pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecStart & SEC_PER_LPN_MSK);
            }

            if ((ucDMATotalLength - g_remainSecToXfer) == ucSecLenInFirst4K)
            {
                *pNcq1stEn = TRUE;
            }
        }
        else
        {
            if (0 == g_remainSecToXfer)
            {
                *pNcq1stEn = TRUE;
            }
        }
    }

    return bUpdateBm;
}

// The goal of this function is analyzing given DSG and give back a memory address according to the
// count of request bytes. In order to avoid gobal variable usage, parameter "pDsgId" and
// "pTakenAwayBytes" must be defined in caller's scope, and pass them to this function each call.
// These two parameters are only be used by this function, caller shouldn't change or use it.
U32 NfcM_GetBufferAddrFromDsg(U32 ulRequestBytes, U32 *pDsgId, U32 *pTakenAwayBytes)
{
    ASSERT((pDsgId != NULL) && (*pDsgId < NORMAL_DSG_NUM));

    U32 ulDsgId = *pDsgId;
    NORMAL_DSG_ENTRY tNormalDsgEntry;
    while (FALSE == DSG_IsNormalDsgValid(ulDsgId)) {}
    DSG_FetchNormalDsg(ulDsgId, &tNormalDsgEntry);
    U32 ulTotalBytesOfCurDsg = tNormalDsgEntry.bsXferByteLen;

    ASSERT((pTakenAwayBytes != NULL) && (*pTakenAwayBytes < ulTotalBytesOfCurDsg));
    ASSERT(ulRequestBytes <= ulTotalBytesOfCurDsg - *pTakenAwayBytes);
    ASSERT((tNormalDsgEntry.bsLast == TRUE) || (tNormalDsgEntry.bsLast == FALSE));

    U32 ulTakenAwayBytes = *pTakenAwayBytes;
    U32 ulResultBufferAddr = (tNormalDsgEntry.bsDramAddr << 1) + ulTakenAwayBytes;
    ulTakenAwayBytes += ulRequestBytes;

    if (ulTakenAwayBytes == ulTotalBytesOfCurDsg)
    {
        // This is the Last data block of the memory space described by this DSG entry, so we need to
        // move to next DSG entry of this DSG entry chain and release this DSG entry.
        U32 ulFinishedDsgId = ulDsgId;

        if (FALSE == tNormalDsgEntry.bsLast)
        {
            ulDsgId = tNormalDsgEntry.bsNextDsgId;
            ulTakenAwayBytes = 0;
        }
        else
        {
            // Set to invalid value in order to catch the wrong usage.
            ulDsgId = NORMAL_DSG_NUM;
            ulTakenAwayBytes = ulTotalBytesOfCurDsg;
        }

        // Clear XORE engine after the current DSG transfer done
        if (tNormalDsgEntry.bsXorClr == TRUE)
        {
            XorM_IReleaseXore(tNormalDsgEntry.bsXorId);
        }

        DSG_ReleaseNormalDsg(ulFinishedDsgId);
    }

    *pDsgId = ulDsgId;
    *pTakenAwayBytes = ulTakenAwayBytes;
    return ulResultBufferAddr;
}

U32 NFC_GetSpareLPN(U32* pRedData,U8 nLPNInPage)
{
    return ((SIM_NFC_RED *)pRedData)->m_DataRed.aCurrLPN[nLPNInPage];
}

BOOL NFC_ReadByteCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam)
{
    FLASH_PHY flash_phy;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U32 *p_buf = g_aDataBufferOut[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu];
#if 0
    if (BAD_BLK_MARK_COLUMN_POS != pFlashCmdParam->p_nf_cq_entry->aByteAddr.usByteAddr)
    {
        DBG_Getch();
    }
#endif

    // read flash IDB
    pFlashCmdParam->phy_page_req[0].row_addr = pFlashCmdParam->p_nf_cq_entry->atRowAddr[0].bsRowAddr;
    NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->phy_page_req[0].row_addr,
        pFlashCmdParam->phy_page_req[0].part_in_wl, pFlashCmdParam->bsIsTlcMode, &flash_phy);

    *(U8*)p_buf = Mid_Read_FlashIDB(&flash_phy);

    Comm_WriteOtfb(pFlashCmdParam->p_red_addr, BAD_BLK_MARK_BYTE_LEN / sizeof(U32) + BAD_BLK_MARK_BYTE_LEN % sizeof(U32), p_buf);

    /*record command*/
    FlashStsM_SetPreCmdInfo(&flash_phy, pFlashCmdParam);
    return TRUE;
}

// Get the length of redundant from current data syntax, ulRedunLength's unit is Byte.
U32 NfcM_GetAtomRedunLength(const NFCQ_ENTRY *pNfcqEntry)
{
    ASSERT(pNfcqEntry != NULL);
    volatile NFC_DATA_SYNTAX *pDataSyntaxReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    U32 ulDataSyntaxEntryIndex = pNfcqEntry->bsDsIndex;
    return (pDataSyntaxReg->atDSEntry[ulDataSyntaxEntryIndex].bsRedNum + 1) * 8;
}

void NfcM_SendConfigToXore(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level)
{
    ASSERT(lun_location != NULL);

    U32 xore_id = NfcM_GetXoreId(lun_location, cmd_level);
    const NFCQ_ENTRY *nfcq_entry = COM_GetNFCQEntry(lun_location, cmd_level);
    U32 atom_redun_length = NfcM_GetAtomRedunLength(nfcq_entry);

    XorM_IReceiveConfigFromNfc(xore_id, nfcq_entry->bsXorBufId, atom_redun_length,
                               nfcq_entry->bsDCrcEn);
    return;
}

void NfcM_SendConfigToXor(const NFCM_LUN_LOCATION *pLunLocation, U32 ulCmdLevel, U32 ulAtomPageIndex,
                          U32 ulCodeWordIndex, const ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    ASSERT(pLunLocation != NULL);
    ASSERT(pFlashCmdParam != NULL);

    U32 ulXoreId = NfcM_GetXoreId(pLunLocation, ulCmdLevel);
    const NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pLunLocation, ulCmdLevel);
    U32 ulAtomRedunLength = NfcM_GetAtomRedunLength(pNfcqEntry);
    U32 ulPlaneCount = NfcM_GetPlaneCount(pLunLocation, ulCmdLevel);
    U32 ulPageInWl = ulAtomPageIndex / ulPlaneCount;
    U32 ulCwIndexOffset = 0;

    if (pFlashCmdParam->bsIsTlcMode == TRUE && pFlashCmdParam->bsIs6DsgIssue != TRUE)
    {
        ulCwIndexOffset = ulPageInWl * (LOGIC_PG_SZ / CW_INFO_SZ);
    }
    else
    {
        ulCwIndexOffset = pNfcqEntry->bsParPgPos * (LOGIC_PG_SZ / CW_INFO_SZ);
    }

    U32 ulRevisedCwIndex = ulCwIndexOffset + ulCodeWordIndex;
    XorM_IReceiveCwConfig(ulXoreId, ulRevisedCwIndex, pNfcqEntry->bsXorBufId, ulAtomRedunLength,
                          pNfcqEntry->bsDCrcEn);
    return;
}

/*********************************************************************
Function     :NFC_PGReadCMD
Input         :
Output       :none
Description :
Note:
Modify History:
20120208    Pengfei Yu    001: first created
**********************************************************************/
BOOL NFC_PGReadCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param)
{
    FLASH_PHY flash_phy;
    SIM_NFC_RED *pLocalRed;
    SIM_NFC_RED tRedTemp = { 0 };
    U8 pg_type;
    U8 bSecHighDW,bitPosInDW;
    U32 dwToDram;
    U32 secIndex;
    U32* p_buf;
    U32* p_addr = NULL;
    U32* p_addr_red = NULL;
    U16 phyPageIndex;
    BOOL bUpdateBm;
    U8 bitPosOfBm;
    BOOL ubNcq1stEn = FALSE;
    U32 ulBmid;
    U32 bCheckNfcErr = TRUE;
    U8 usErrCode = NF_SUCCESS;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);

    // If enable XOR, NFC need tell some configure information to the XOR engine of current NFC command.
    if (pFlashCmdParam->p_nf_cq_entry->bsXorEn == TRUE)
    {
        NfcM_SendConfigToXore(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct));
    }

    //read whole logic-page out with redundant
    pLocalRed = (SIM_NFC_RED *)pFlashCmdParam->p_local_red;
    p_buf = g_aDataBufferOut[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu];

    for (phyPageIndex = 0; phyPageIndex < pFlashCmdParam->bsPlnNum; phyPageIndex++)
    {
        pFlashCmdParam->phy_page_req[phyPageIndex].row_addr = pFlashCmdParam->p_nf_cq_entry->atRowAddr[phyPageIndex].bsRowAddr;
        pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl = NFC_GetPartInWL(pFlashCmdParam->bsCmdCode);

        NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->phy_page_req[phyPageIndex].row_addr,
            pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl, pFlashCmdParam->bsIsTlcMode, &flash_phy);

        p_addr_red = &pLocalRed->m_Content[g_RED_SZ_DW * phyPageIndex];

        //if (pFlashCmdParam->bsIsChangeReadColumn == FALSE)
        {
            usErrCode = Mid_Read(&flash_phy, (char*)p_buf + phyPageIndex * g_PG_SZ, (char*)p_addr_red, g_PG_SZ, bCheckNfcErr, pFlashCmdParam, tNfcOrgStruct, (phyPageIndex / pFlashCmdParam->bsPlnNum));
        }

        if (TRUE == bCheckNfcErr && NF_SUCCESS != usErrCode)
        {
            usErrCode = NFC_ErrInj(usErrCode, pFlashCmdParam);
        }

        //SystemStatisticRecord("NFC_PGReadCMD: PU=%d, block = %d, page = %d  err = %d, bCheckNfcErr = %d, ParaErrEn = %d, ParaErrType = %d\n", flash_phy.nPU, flash_phy.nBlock, flash_phy.nPage, usErrCode, bCheckNfcErr, pFlashCmdParam->module_inj_en, pFlashCmdParam->module_inj_type);
        bCheckNfcErr = TRUE;

        /*record command*/
        FlashStsM_SetPreCmdInfo(&flash_phy, pFlashCmdParam);
    }

    if(TRUE == pFlashCmdParam->p_nf_cq_entry->bsEMEn)
    {
#ifdef DATA_EM_ENABLE
        if (flash_phy.nBlock >= g_ulDataBlockStart[0][flash_phy.ucLunInTotal])
        {
            COM_MemCpy((U32*)&tRedTemp, (U32*)pLocalRed, RED_SW_SZ_DW);
            NFC_TransEmRed((SIM_NFC_RED *)pLocalRed, &tRedTemp);
        }
#endif
    }

    //judge data type
    pg_type = Flash_Talbe_GetPageType(&flash_phy);
    dwToDram = (pg_type == COM_DATA_TYPE) ? 2 : SEC_SIZE / sizeof(U32);

    if (TRUE == pFlashCmdParam->bsIsTlcMode)
    {
        //DBG_Printf("NFCRead PU %d PhyBlk %d Pg %d PgType %d pLocalRed 0x%x \n", flash_phy.ucLunInTotal, flash_phy.nBlock, flash_phy.nPage, pg_type, (U32)pLocalRed);
    }

    if(TRUE == pFlashCmdParam->p_nf_cq_entry->bsEMEn)
    {
#ifdef DATA_EM_ENABLE
        if (flash_phy.nBlock >= g_ulDataBlockStart[0][flash_phy.ucLunInTotal])
        {
            COM_MemCpy((U32*)pLocalRed, (U32*)&tRedTemp, RED_SW_SZ_DW);
        }
#endif
    }

    //FIRMWARE_LogInfo("NFCRead PU %d PhyBlk %d Pg %d PgType %d pLocalRed 0x%x \n", flash_phy.ucLunInTotal, flash_phy.nBlock, flash_phy.nPage, pg_type, pLocalRed);

    //copy data to DMA_buffer
    initDmaAddr(pFlashCmdParam->p_nf_cq_entry);

    for (phyPageIndex = 0; phyPageIndex < pFlashCmdParam->bsPlnNum; phyPageIndex++)
    {
        if((pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[0] != 0
            || pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[1] != 0)
            && pFlashCmdParam->red_only != 1)
        {
            p_buf = g_aDataBufferOut[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu] + phyPageIndex*g_PG_SZ / sizeof(U32);

            secIndex = 0;
            while (secIndex < SEC_PER_PHYPG)
            {
                bSecHighDW = (secIndex>31)? 1 : 0;
                bitPosInDW = (bSecHighDW==1)?(secIndex-32):secIndex;
                if(pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[bSecHighDW] & (1<<bitPosInDW))
                {
                    U16 usReleaseDsgFlag;
                    usReleaseDsgFlag = 0;// bit15 = 1, need to release DSG; bit[14:0] = DSG id to release
                    ubNcq1stEn = FALSE;
                    bUpdateBm = NFC_GetDMATargetAddr(pFlashCmdParam, &p_addr, &bitPosOfBm, &ulBmid, &ubNcq1stEn, &usReleaseDsgFlag);

                    if (NFCM_PAGE_DATA_IN_DRAM == NfcM_GetPageDataLocation(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct), phyPageIndex))
                    {
                        if (pFlashCmdParam->p_nf_cq_entry->bsXorEn == TRUE)
                        {
                            // XOR engine calculate 1 Code-Word data each time, and sector's size is half of Code-Words',
                            // so call XOR model when "ulSecCounter" is odd.
                            if (secIndex % 2 == 1)
                            {
                                U32 ulXoreId = NfcM_GetXoreId(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct));

                                // Redundant of current page need to calculate with the last Code-Word.
                                if (secIndex == ((LOGIC_PG_SZ / SEC_SIZE) - 1))
                                {
                                    // @todo "RED_SZ" shouldn't at here, this will be improved later.
                                    U32 *pLastCwAndRedun = (U32 *)malloc(CW_INFO_SZ + RED_SZ);
                                    if (pLastCwAndRedun == NULL)
                                    {
                                        DBG_Printf("File:%s Line:%d Memory allocation failed!", __FILE__, __LINE__);
                                        DBG_Getch();
                                    }

                                    memcpy(pLastCwAndRedun, (p_buf + (SEC_SIZE * (secIndex - 1)) / sizeof(U32)), CW_INFO_SZ);
                                    memcpy((pLastCwAndRedun + CW_INFO_SZ_DW), &(pLocalRed->m_Content[g_RED_SZ_DW * phyPageIndex]), RED_SZ);

                                    XORM_ICalculateNFC(ulXoreId, pLastCwAndRedun, (secIndex / 2));
                                    free(pLastCwAndRedun);
                                }
                                else
                                {
                                    // "pCodeWordData" need to point to previous sector's buffer.
                                    const U32 * pCodeWordData = p_buf + (SEC_SIZE * (secIndex - 1)) / sizeof(U32);
                                    XORM_ICalculateNFC(ulXoreId, pCodeWordData, (secIndex / 2));
                                }
                            }
                        }

                        Comm_WriteDram((U32)p_addr, dwToDram, p_buf + (SEC_SIZE*secIndex) / sizeof(U32));
                    }
                    else
                    {
                        Comm_WriteOtfb((U32)p_addr, dwToDram, p_buf + (SEC_SIZE*secIndex) / sizeof(U32));
                    }

                    #ifdef HOST_SATA
                    if(TRUE == bUpdateBm) //update buffmap
                    {
                        if ((NF_SUCCESS == pFlashCmdParam->module_inj_type) || (NF_ERR_TYPE_RECC == pFlashCmdParam->module_inj_type))
                        {
                            pFlashCmdParam->buffMapValue = 0;
                            pFlashCmdParam->buffMapValue |= (1<<bitPosOfBm);
                            NFC_InterfaceSetBuffermap(ucPhyPuInTotal, pFlashCmdParam->buffMapValue, ulBmid);
                            //SystemStatisticRecord("CE %d Blk %d Page %d BufMapID %d \n", ucppu, flash_phy.nBlock, flash_phy.nPage, ulBmid);
                        }
                    }
                    #endif

                    if (0 != (usReleaseDsgFlag&(1<<15)))
                    {
                        DSG_ReleaseNormalDsg(usReleaseDsgFlag & (~(1<<15)));
                    }

                    //update Ncq1st
                    if(TRUE == ubNcq1stEn)
                    {
                        NFC_InterfaceSetFirstDataReady(pFlashCmdParam->ncq_num);
                    }
                }
                secIndex++;
            }
        }
    }


    return TRUE;
}

U32 NFC_GetLBAInSubSystem(U32 ulSrcLBA)
{
    U8 ulSubSysNumBits = 0;
    U32 ulLanePerBuf = 0;

    ulSubSysNumBits = 0;

    ulLanePerBuf = ulSrcLBA >> (SEC_PER_BUF_BITS + ulSubSysNumBits);
    ulSrcLBA = (ulLanePerBuf << SEC_PER_BUF_BITS) + (ulSrcLBA & SEC_PER_BUF_MSK);

    return ulSrcLBA;
}

U32 NFC_GetLBAInSystem(U32 SubSystemLpn)
{
    U32 SubSystemLba,ulSystemLba;
    U32 ulSubSysNumBits;

    ulSubSysNumBits = 0;

    SubSystemLba = SubSystemLpn*SEC_PER_LPN;

    ulSystemLba = SubSystemLba;

    return ulSystemLba;
}
U32 g_DebugCnt = 0;
BOOL NFC_CheckData(U32 *pData, U32 LPNInSpare,U32 SecOffsetInLPN, U32 OPType)
{
    ERR_INJ_PARRM* pCRCErrParam;
    U32 ulLBA = INVALID_8F ;
    U32 ulWriteCnt = INVALID_8F;
    U32 ulLBAInSubSys = INVALID_8F;
    BOOL bEqual = TRUE;
    U8 ucIndex;

    if (NULL == pData)
    {
        DBG_Break();
    }

    ulWriteCnt = *pData;
    ulLBA = *(pData + 1);

    if((INVALID_8F == ulLBA) || (INVALID_8F == LPNInSpare))
    {
        return TRUE;
    }

    for(ucIndex = 0; ucIndex < g_InjCRCErrTail; ucIndex++)
    {
        pCRCErrParam = &CRCErrParamSlot[ucIndex];

        if ((CMD_CODE_PROGRAM == pCRCErrParam->CmdCode) && (ulLBA == pCRCErrParam->LBA))
        {
            CRCErrParamSlot[ucIndex].bHit = TRUE;
        }
    }


#ifdef DBG_TABLE_REBUILD
    if((TRUE != g_bReSetFlag) && (TRUE != IsDuringTableRebuild()))
    {
        /*modidy by tobey 20160125, for corner case:
        WL tirgger HighBit Clear with old LBA info which may result in host data Cnt Check error*/
        if ((OP_TYPE_HOST_WRITE == OPType) && (0 != Host_GetDataCntHighBit(ulLBA)))
        {
            Host_ClearDataCntHighBit(ulLBA,ulWriteCnt);
        }
    }
#endif

    ulLBAInSubSys = NFC_GetLBAInSubSystem(ulLBA);

    if ((ulLBAInSubSys/SEC_PER_LPN) != LPNInSpare)
    {
        /*add by nina 2014-10-13,because this corner case will be checked error
        1.write LPN0 to Addr1;
        2.Trim LPN0,PMT is INVALID_8F;
        3.Addr1 is erased and written new data;
        4.abnormal shutdown,
        a)rebuild PMT,LPN0 -> old Addr1
        b)Host write LPN0,and need merge some lba,
        because Addr1 is writtern new data,nfc will check data error.
        So add this to ignore this check,and clear buffer to INVALID_8F*/
#ifdef DBG_TABLE_REBUILD
        U32 ulSystemLba = NFC_GetLBAInSystem(LPNInSpare) + SecOffsetInLPN;
        if((0 == Host_GetDataCnt(ulSystemLba)) ||
            (TRUE == Host_IsHitHCmdTable(ulSystemLba) && (Host_GetDataCnt(ulSystemLba) > 0)) ||
            IsDuringRebuildDirtyCnt(MCU1_ID))
        {
            DBG_Printf("Ignore check LPNInSpare 0x%x ulSystemLba 0x%x LBAInBuffer 0x%x\n",
                LPNInSpare,ulSystemLba,ulLBA);
            *pData = INVALID_8F;
            *(pData + 1) = INVALID_8F;
            g_DebugCnt++;
            return TRUE;
        }
        else
        {
            DBG_Printf("ulSystemLba 0x%x WriteCnt %d\n",ulSystemLba,Host_GetDataCnt(ulSystemLba));
        }
#endif
        DBG_Printf("LBAInBuffer 0x%x, LPNinSubSys 0x%x != LPNInSpare 0x%x \n", ulLBA, ulLBAInSubSys / SEC_PER_LPN, LPNInSpare);
        DBG_Break();
        bEqual = FALSE;
    }

    return bEqual;
}

//#ifdef FLASH_TLC
/*********************************************************************
Function        :NFC_SlcCopyToSlcCMD
Input        :
Output        :none
Description :
Note:
Modify History:

**********************************************************************/
BOOL NFC_SlcCopyToSlcCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam)
{
    FLASH_PHY flash_phy_source;
    FLASH_PHY flash_phy_target;
    U8 ucPhyPageIndex;
    U32* p_addr_red = NULL;
    SIM_NFC_RED *pLocalRed;
    U8 pg_type;

    for (ucPhyPageIndex = 0; ucPhyPageIndex < PLN_PER_LUN; ucPhyPageIndex++)
    {
        NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->p_nf_cq_entry->atRowAddr[ucPhyPageIndex].bsRowAddr, 0, FALSE, &flash_phy_source);
        NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->p_nf_cq_entry->atRowAddr[ucPhyPageIndex + PLN_PER_LUN].bsRowAddr, 0, FALSE, &flash_phy_target);

        pLocalRed = (SIM_NFC_RED *)pFlashCmdParam->p_local_red;
        p_addr_red = &pLocalRed->m_Content[g_RED_SZ_DW * ucPhyPageIndex];
        Mid_Read_RedData(&flash_phy_source, (char*)p_addr_red);

        if (0 == ucPhyPageIndex)
        {
            pg_type = NFC_GetSparePGType((SIM_NFC_RED *)p_addr_red);
        }

        if (PG_TYPE_DATA_SIMNFC == pg_type)
        {
            Mid_CopyData(&flash_phy_source, &flash_phy_target, FALSE);
        }
        else
        {
            Mid_CopyTableData(&flash_phy_source, &flash_phy_target, FALSE);
        }
    }

    return TRUE;
}

/*********************************************************************
Function        :NFC_SlcCopyToSlcCMD
Input        :
Output        :none
Description :
Note:
Modify History:

**********************************************************************/
BOOL NFC_SlcCopyToTlcCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam, U8 ucTriggerStage, U8 ucPartInWL)
{
    FLASH_PHY flash_phy_source;
    FLASH_PHY flash_phy_target;
    U8 ucSLCIndex, ucPhyPageIndex;
    U8 ucPageInWL;
    BOOL bWriteToFlash;
    U32 *p_buf = NULL;
    U32* p_addr_red = NULL;
    U8 pg_type;
    SIM_NFC_RED *pLocalRed;
    ST_FLASH_CMD_PARAM* p_flash_param = pFlashCmdParam;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    p_buf = g_aDataBufferOut[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu];

    for (ucPhyPageIndex = 0; ucPhyPageIndex < PLN_PER_LUN; ucPhyPageIndex++)
    {
        for (ucSLCIndex = 0; ucSLCIndex < ucTriggerStage; ucSLCIndex++)
        {
            ucPageInWL = (ucTriggerStage == PAGE_PER_WL) ? ucSLCIndex : (pFlashCmdParam->bsCmdCode - NF_PRCQ_SLC_COPY_TLC_1ST_MULTIPLN) % PAGE_PER_WL;

            NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->p_nf_cq_entry->atRowAddr[ucPhyPageIndex + ucSLCIndex * PLN_PER_LUN].bsRowAddr, 0, FALSE, &flash_phy_source);
            NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->p_nf_cq_entry->atRowAddr[ucPhyPageIndex + ucTriggerStage * PLN_PER_LUN].bsRowAddr, ucPageInWL, TRUE, &flash_phy_target);

            pLocalRed = (SIM_NFC_RED *)pFlashCmdParam->p_local_red;
            p_addr_red = &pLocalRed->m_Content[g_RED_SZ_DW * (ucPhyPageIndex + ucSLCIndex* PLN_PER_LUN)];
            Mid_Read_RedData(&flash_phy_source, (char*)p_addr_red);

            if (0 == ucPhyPageIndex)
            {
                pg_type = NFC_GetSparePGType((SIM_NFC_RED *)p_addr_red);
                if (PG_TYPE_FREE == pg_type)
                {
                    DBG_Printf("The Pu:%d, SLC blk:%d is no data blk, can't internal copy to TLC blk\n", flash_phy_source.ucLunInTotal, flash_phy_source.nBlock);
                    DBG_Getch();
                }
            }

            if(g_tFlashSpecInterface.CheckBlkType != NULL &&
                FALSE == g_tFlashSpecInterface.CheckBlkType(flash_phy_source.ucLunInTotal, flash_phy_source.nPln, flash_phy_source.nBlock, CMD_CODE_READ, FALSE))
            {
                DBG_Printf("The souce block is not SLC block\n");
                DBG_Getch();
            }

            if (0 != ucPartInWL)
            {
                p_flash_param->bsCmdType = CMD_CODE_READ;
                p_flash_param->bsIsTlcMode = TRUE;

                Mid_Read(&flash_phy_target, (char*)p_buf, (char*)p_addr_red, g_PG_SZ, TRUE, p_flash_param, tNfcOrgStruct, (ucPhyPageIndex / PLN_PER_LUN));
            }

           /* check program order */
           if(NULL != g_tFlashSpecInterface.CheckPrgOrder)
            {
               bWriteToFlash = g_tFlashSpecInterface.CheckPrgOrder(&flash_phy_target, (char*)p_buf, (char*)p_addr_red, COM_DATA_TYPE, ucPartInWL, p_flash_param);
            }

            if (bWriteToFlash == TRUE)
            {
                U8 ucErrCode = Mid_CopyData(&flash_phy_source, &flash_phy_target, TRUE);
                if (NF_SUCCESS != ucErrCode && ucPhyPageIndex==0)
                {
                    NFC_ErrInj(ucErrCode, pFlashCmdParam);
                }
            }
        }
    }

    return TRUE;
}
//#endif

// Redundant of XOR parity page is stored in OTFB SRAM, and not stay together with other
// redundant of this command. The address is relative to OTFB_START_ADDRESS.
U32 NfcM_GetAtomXorParityRedunAddr(const NFCQ_ENTRY *pNfcqEntry, U32 ulPageInWl)
{
    ASSERT(pNfcqEntry != NULL);
    ASSERT(ulPageInWl < PAGE_PER_WL);

    U32 ulRedunOtfbOffset = 0;

    switch (ulPageInWl)
    {
        case 0:
        {
            ulRedunOtfbOffset = pNfcqEntry->bsRedAddXorPar0 << XOR_REDUN_ALIGN_BITS ;
            break;
        }
        case 1:
        {
            ulRedunOtfbOffset = pNfcqEntry->bsRedAddXorPar1 << XOR_REDUN_ALIGN_BITS ;
            break;
        }
        case 2:
        {
            ulRedunOtfbOffset = pNfcqEntry->bsRedAddXorPar2 << XOR_REDUN_ALIGN_BITS ;
            break;
        }
        default:
            ASSERT(FALSE);
    }

    return (OTFB_XOR_REDUNDANT_BASE + ulRedunOtfbOffset - OTFB_START_ADDRESS);
}

// Atom page is single plane page for MLC, is a single plane lower page, middle page or upper page
// for TLC. Atom redundant is the redundant of atom page.
void NfcM_ReadAtomRedun(const NFCM_LUN_LOCATION *pLunLocation, U32 ulCmdLevel, U32 ulAtomPageIndex,
                        U32 ulAtomRedunLengthMax, U8 *pAtomRedun)
{
    ASSERT(pAtomRedun != NULL);

    const NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pLunLocation, ulCmdLevel);
    U32 ulAtomRedunLength = NfcM_GetAtomRedunLength(pNfcqEntry);
    ASSERT(ulAtomRedunLength <= ulAtomRedunLengthMax);
    U32 ulPlaneCountOfCurrentCmd = NfcM_GetPlaneCount(pLunLocation, ulCmdLevel);
    U32 ulPlaneIndex = ulAtomPageIndex % ulPlaneCountOfCurrentCmd;
    U32 ulPageInWl = ulAtomPageIndex / ulPlaneCountOfCurrentCmd;
    // This address is relative to OTFB_START_ADDRESS or DRAM_START_ADDRESS.
    U32 ulAtomRedunAddr = 0;

    if (TRUE == NfcM_IsXorParityPageInWriteCmd(pLunLocation, ulCmdLevel, ulPlaneIndex))
    {
        // When XOR is enabled, the redundant of XOR parity page is stored in OTFB.
        ulAtomRedunAddr = NfcM_GetAtomXorParityRedunAddr(pNfcqEntry, ulPageInWl);
        Comm_ReadOtfb(ulAtomRedunAddr, ulAtomRedunLength / sizeof(U32), (U32 *)pAtomRedun);
    }
    else
    {
        // Excepting the redundant of XOR parity page, other redundant are stored one by one.
        ulAtomRedunAddr = NfcM_Get1stAtomRedunOffset(pLunLocation, ulCmdLevel) +
                ulAtomPageIndex * ulAtomRedunLength;
        if (TRUE == pNfcqEntry->bsRedOntf)
        {
            Comm_ReadOtfb(ulAtomRedunAddr, ulAtomRedunLength / sizeof(U32), (U32 *)pAtomRedun);
        }
        else
        {
            ulAtomRedunAddr += g_ulModelRedOffSetInDram;
            Comm_ReadDram(ulAtomRedunAddr, ulAtomRedunLength / sizeof(U32), (U32 *)pAtomRedun);
        }
    }
    return;
}

// Entire redundant is comprised of atom redundant of all planes. For 2 planes 2D TLC, there are
// three entire redundant in one Word-Line, lower page entire redundant = atom redundant of plane0
// lower page + atom redundant of plane1 lower page. We prefer a write data check during reading
// origin data from memory space, so we need read entire redundant first.
void NfcM_ReadEntireRedun(const NFCM_LUN_LOCATION *pLunLocation, U32 ulCmdLevel,
                          U32 ulPlane0AtomPageIndex, SIM_NFC_RED *pEntireRedun)
{
    ASSERT(pLunLocation != NULL);
    ASSERT(pEntireRedun != NULL);
    const NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pLunLocation, ulCmdLevel);
    U32 ulAtomRedunLength = NfcM_GetAtomRedunLength(pNfcqEntry);
    U32 ulPlaneCountOfCurrentCmd = NfcM_GetPlaneCount(pLunLocation, ulCmdLevel);

    for (U32 i = 0; i < ulPlaneCountOfCurrentCmd; ++i)
    {
        U8 aucAtomRedun[NFCM_MAX_REDUN_SZ] = {0};
        NfcM_ReadAtomRedun(pLunLocation, ulCmdLevel, (i + ulPlane0AtomPageIndex), sizeof(aucAtomRedun),
                           aucAtomRedun);
        memcpy(&pEntireRedun->m_Content[i * (ulAtomRedunLength / sizeof(U32))], aucAtomRedun,
               ulAtomRedunLength);
    }
    return;
}

void NfcM_NotifyOtherModulesMonitorDram(const NFCM_LUN_LOCATION *pLunLocation, U32 ulCmdLevel,
                                        U32 ulAtomPageIndex, U32 ulCodeWordIndex,
                                        const ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    ASSERT(pLunLocation != NULL);

    const NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pLunLocation, ulCmdLevel);
    if (pNfcqEntry->bsXorEn == TRUE)
    {
        // If enable XOR, NFC need tell some configure information and the index of current Code-Word
        // to the XOR engine of current NFC command.
        NfcM_SendConfigToXor(pLunLocation, ulCmdLevel, ulAtomPageIndex, ulCodeWordIndex,
                             pFlashCmdParam);
    }
    return;
}

void NfcM_CheckLbaWithLpn(const SIM_NFC_RED *pRedunForCheck, U8 *pAtomPage, U32 ulPlaneIndex, U32 ulCwIndex, FLASH_PHY *pFlashAddr)
{
    U32 ulRedOPType = NFC_GetSpareOpType((U32 *)&pRedunForCheck);
    U8 pg_type = NFC_GetSparePGType(pRedunForCheck);
    U32 nType = (PG_TYPE_DATA_SIMNFC == pg_type) ? COM_DATA_TYPE : RSV_DATA_TYPE;

    if (COM_DATA_TYPE == nType)
    {
        U32 ulSecIndex = ulCwIndex * (CW_INFO_SZ / SEC_SIZE);
        for (U32 i = 0; i < (CW_INFO_SZ / SEC_SIZE); ++i)
        {
            U32 ulLpnIndex = (ulPlaneIndex * SEC_PER_PHYPG + ulSecIndex) / SEC_PER_LPN;
            if (FALSE == NFC_CheckData((U32 *)&pAtomPage[SEC_SIZE * ulSecIndex],
                                       pRedunForCheck->m_DataRed.aCurrLPN[ulLpnIndex],
                                       ulSecIndex % SEC_PER_LPN,
                                       ulRedOPType))
            {
                DBG_Printf("NFC_PGWriteCMD:LUN %d Blk 0x%x Pg 0x%x PgType %d pLocalRed 0x%x Check Error\n", pFlashAddr->ucLunInTotal, pFlashAddr->nBlock, pFlashAddr->nPage, pg_type, (U32)&pRedunForCheck);
                DBG_Break();
            }
            ++ulSecIndex;
        }
    }
    return;
}

void NFC_TransEmRed(SIM_NFC_RED* pRedDst, SIM_NFC_RED* pRedSrc)
{
    U32 Pln;
    U32* pDst = (U32*)pRedDst;
    U32* pSrc;

    for (Pln = 0; Pln < PLN_PER_LUN; Pln++)
    {
        pSrc = (U32*)pRedSrc + 4 + RED_SZ_DW*Pln;
        COM_MemCpy(pDst, pSrc, (RED_SZ_DW - 4));
        pDst += (RED_SZ_DW - 4);
    }
}
// Atom page is single plane page for MLC, is a single plane lower page, middle page or upper page
// for TLC. Atom redundant is the redundant of atom page.
BOOL NFC_PGWriteCMD(const NFCM_LUN_LOCATION *pLunLocation, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U32 ulCmdLevel = NFC_GetRP(pLunLocation);
    NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pLunLocation, ulCmdLevel);
    U32 ulCurDsgId = pNfcqEntry->bsFstDsgPtr;
    U32 ulTakenAwayBytes = 0;
    SIM_NFC_RED tRedunForCheck = { 0 }, tRedTemp = { 0 };
    U32 ulAtomPageCount = (pNfcqEntry->bsDmaTotalLength * CW_INFO_SZ) / LOGIC_PG_SZ;
    U32 ulCurCwAddr, ulCwIndex;
    FLASH_PHY tFlashPhy = { 0 };

    // Each loop complete all things of one atom page.
    for (U32 ulAtomPageIndex = 0; ulAtomPageIndex < ulAtomPageCount; ++ulAtomPageIndex)
    {
        U8 aucAtomPage[LOGIC_PG_SZ] = { 0 };
        U8 aucAtomRedun[NFCM_MAX_REDUN_SZ] = {0};
        U32 ulPlaneCount = NfcM_GetPlaneCount(pLunLocation, ulCmdLevel);
        U32 ulPlaneIndex = ulAtomPageIndex % ulPlaneCount;
        U32 ulPageInWl = (pFlashCmdParam->bsIs6DsgIssue == TRUE) ? pNfcqEntry->bsParPgPos : ulAtomPageIndex / ulPlaneCount;

        // Read entire redundant of current "PIPE" page for data check usage. We must read this before
        // atom page data being read, because we can use a member of a structure only when the
        // structure is entire.
        if (ulPlaneIndex == 0)
        {
            NfcM_ReadEntireRedun(pLunLocation, ulCmdLevel, ulAtomPageIndex, &tRedunForCheck);
        }

        if (g_tFlashSpecInterface.GetWriteFlashAddr != NULL)
        {
            g_tFlashSpecInterface.GetWriteFlashAddr(pLunLocation, pFlashCmdParam, ulPlaneIndex, ulAtomPageIndex, ulPageInWl, pFlashCmdParam->bsIsTlcMode, &tFlashPhy);
        }

        if(TRUE == pFlashCmdParam->p_nf_cq_entry->bsEMEn)
        {
#ifdef DATA_EM_ENABLE
            if (tFlashPhy.nBlock >= g_ulDataBlockStart[0][tFlashPhy.ucLunInTotal] && ulPlaneIndex == 0)
            {
                COM_MemCpy((U32*)&tRedTemp, (U32*)&tRedunForCheck, RED_SW_SZ_DW);
                NFC_TransEmRed(&tRedunForCheck, &tRedTemp);
            }
#endif
        }

        // Read whole data of current atom page Code-Word by Code-Word from outside memory space to NFC
        // model.
        for (ulCwIndex = 0; ulCwIndex < (LOGIC_PG_SZ / CW_INFO_SZ); ++ulCwIndex)
        {
            ulCurCwAddr = NfcM_GetBufferAddrFromDsg(CW_INFO_SZ, &ulCurDsgId, &ulTakenAwayBytes);

            if (NFCM_PAGE_DATA_IN_DRAM == NfcM_GetPageDataLocation(pLunLocation, ulCmdLevel, ulPlaneIndex))
            {
                NfcM_NotifyOtherModulesMonitorDram(pLunLocation, ulCmdLevel, ulAtomPageIndex, ulCwIndex, pFlashCmdParam);
                Comm_ReadDram(ulCurCwAddr, CW_INFO_SZ_DW, (U32 *)&aucAtomPage[CW_INFO_SZ * ulCwIndex]);
            }
            else
            {
                Comm_ReadOtfb(ulCurCwAddr, CW_INFO_SZ_DW, (U32 *)&aucAtomPage[CW_INFO_SZ * ulCwIndex]);
            }

            // Check LBA in buffer with LPN in spare
            NfcM_CheckLbaWithLpn(&tRedunForCheck, aucAtomPage, ulPlaneIndex, ulCwIndex, &tFlashPhy);
        }

        // Read the redundant of current atom page from outside memory space to NFC model.
        NfcM_ReadAtomRedun(pLunLocation, ulCmdLevel, ulAtomPageIndex, sizeof(aucAtomRedun), aucAtomRedun);

        U32 nType = (PG_TYPE_DATA_SIMNFC == NFC_GetSparePGType(&tRedunForCheck)) ? COM_DATA_TYPE : RSV_DATA_TYPE;

        U8 ucErrCode = Mid_Write(&tFlashPhy, nType, aucAtomPage, aucAtomRedun, g_PG_SZ, TRUE,
                                 pFlashCmdParam, pLunLocation, ulAtomPageIndex / ulPlaneCount);
        if (NF_SUCCESS != ucErrCode)
        {
            NFC_ErrInj(ucErrCode, pFlashCmdParam);
        }

    }

    return TRUE;
}

/*********************************************************************
Function        :NFC_BlkEreaseCMD
Input        :
Output        :none
Description :
Note:
Modify History:
20120208    Pengfei Yu    001: first created
**********************************************************************/
BOOL NFC_BlkEreaseCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam)
{
    FLASH_PHY flash_phy;
    U32 rowAddr;
    U32 VirBlk = INVALID_4F;
    U8 planeIndex;
    U8 usErrCode;
    U8 ucCmdType, ucStartPln, ucEndPln;
    U8 bCheckNFCErr = TRUE;
    BOOL bsLLF = TRUE;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);

    rowAddr = pFlashCmdParam->p_nf_cq_entry->atRowAddr[0].bsRowAddr;
    NFC_GetFlashAddr(tNfcOrgStruct, rowAddr, 0, pFlashCmdParam->bsIsTlcMode, &flash_phy);

#ifdef DBG_TABLE_REBUILD
    SIM_NFC_RED red = { 0 };
    Mid_Read_RedData(&flash_phy, (char*)&red);
    VirBlk = red.m_RedComm.bsVirBlockAddr;
#endif

    ucCmdType = NFC_GetCmdCode(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct));
    if (NF_PRCQ_ERS == ucCmdType || NF_PRCQ_TLC_ERS == ucCmdType)
    {
        ucStartPln = flash_phy.nPln;
        ucEndPln = ucStartPln + 1;
        bsLLF = TRUE;
    }
    else
    {
        ucStartPln = 0, ucEndPln = PLN_PER_LUN;
        bsLLF = FALSE;
    }

    for (planeIndex = ucStartPln; planeIndex < ucEndPln; planeIndex++)
    {
        if (NF_PRCQ_ERS == ucCmdType || NF_PRCQ_TLC_ERS == ucCmdType)
        {
            flash_phy.nPln = planeIndex;
        }
        else
        {
            rowAddr = pFlashCmdParam->p_nf_cq_entry->atRowAddr[planeIndex].bsRowAddr;
            NFC_GetFlashAddr(tNfcOrgStruct, rowAddr, 0, pFlashCmdParam->bsIsTlcMode, &flash_phy);
        }

        usErrCode = Mid_Erase(&flash_phy, bCheckNFCErr, bsLLF, pFlashCmdParam, tNfcOrgStruct);
        if (NF_SUCCESS != usErrCode && TRUE == bCheckNFCErr)
        {
            usErrCode = NFC_ErrInj(usErrCode, pFlashCmdParam);
        }
        bCheckNFCErr = TRUE;

        // Hi Leo, we need to double check that the error injection is located on the block-in-pln-level.
        NFC_ClearInjErrEntry((U8)(flash_phy.ucLunInTotal), flash_phy.nBlock, INVALID_4F);
    }

#ifdef DBG_TABLE_REBUILD
    L2_RemoveErrAddrOfCheckPMT(flash_phy.ucLunInTotal, VirBlk);
#endif

    return TRUE;
}

#if 0
U32 g_TotalBlkBits = 0;
void NFC_CalcBlkBits()
{
    U32 ulTotalBlk = BLK_PER_PLN + RSV_BLK_PER_PLN;
    U32 i = 0;

    if (RSV_BLK_PER_PLN == 0)
    {
        g_TotalBlkBits = BLK_PER_PLN_BITS;
    }
    else
    {
        g_TotalBlkBits = 0;
        while ((ulTotalBlk >> i) != 0)
        {
            i++;
        }
        g_TotalBlkBits = i;
    }
    return;
}
#endif
/*********************************************************************
Function        :NFC_GetFlashAddr()
Input        :
Output        :none
Description :get the cq addr from NFC phy address to FTL phy address
Note:
Modify History:
20120208    Pengfei Yu    001: first created
**********************************************************************/
void NFC_GetFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, U32 row_addr, U8 part_in_wl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr)
{
    U8  pln, plun, rp=0, ceSel=0;
    U16 pbn, ppo;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);

    //ppo = (row_addr & WL_ADDR_MSK) * PAGE_PER_WL + part_in_wl;

/*  comment by abby
#ifdef FLASH_TLC
    //pbn = ((row_addr >> BLK_POS_IN_ROW_ADDR) & BLK_ADDR_MSK);
    //pbn = ((row_addr >> (WL_ADDR_BITS + LOGIC_PLN_PER_LUN_BITS)) & BLK_ADDR_MSK);
    //pln = ((row_addr >> (WL_ADDR_BITS + 1)) & LOGIC_PLN_PER_LUN_MSK);
    //pln = ((row_addr >> PLN_POS_IN_ROW_ADDR) & LOGIC_PLN_PER_LUN_MSK);
    //pln = ((row_addr >> (WL_ADDR_BITS)) & LOGIC_PLN_PER_LUN_MSK);
#else
    //pbn = ((row_addr >> (WL_ADDR_BITS + LOGIC_PLN_PER_LUN_BITS)) & BLK_ADDR_MSK);
    //pln = ((row_addr >> WL_ADDR_BITS) & LOGIC_PLN_PER_LUN_MSK);
#endif
*/
#ifdef FLASH_INTEL_3DTLC
    if (FALSE == IsTLCMode && TRUE == g_bSquashMode)
    {
        ppo = (row_addr & ((1 << SLC_PG_PER_BLK_BITS) - 1)) * PAGE_PER_WL + part_in_wl;
        pbn = ((row_addr >> SLC_BLK_POS_IN_ROW_ADDR) & BLK_PER_PLN_MSK);
        pln = ((row_addr >> SLC_PLN_POS_IN_ROW_ADDR) & PLN_PER_LUN_MSK);
    }
    else
#endif
    {
        ppo = (row_addr & ((1 << PG_PER_BLK_BITS) - 1)) * PAGE_PER_WL + part_in_wl;
        pbn = ((row_addr >> BLK_POS_IN_ROW_ADDR) & BLK_PER_PLN_MSK);
        pln = ((row_addr >> PLN_POS_IN_ROW_ADDR) & PLN_PER_LUN_MSK);
    }
    plun = (row_addr >> LUN_POS_IN_ROW_ADDR);

    pFlashAddr->nBlock = pbn;
    pFlashAddr->nPln = pln;
    pFlashAddr->nPage = ppo;
    pFlashAddr->bsPhyPuInTotal = ucPhyPuInTotal;
    pFlashAddr->bsLunInCe = plun;

    rp = NFC_GetRP(tNfcOrgStruct);
    ceSel = (U8)(NfcM_GetTriggerReg(tNfcOrgStruct, rp)->bsCESel);

    pFlashAddr->ucLunInTotal = (pFlashAddr->bsPhyPuInTotal * LUN_PER_CE * NFC_CE_PER_PU + ceSel * LUN_PER_CE + pFlashAddr->bsLunInCe);

    return;
}

U8 NFC_GetSparePGType(const SIM_NFC_RED *p_red)
{
    return p_red->m_RedComm.bcPageType;
}

U32 NFC_GetSpareOpType(U32* pRedData)
{
    return ((SIM_NFC_RED *)pRedData)->m_RedComm.eOPType;
}

void mem_set
(
 U32 dst_addr,
 U32 cnt_dw,
 U32 val
 )
{
    U32 dw;
    for( dw = 0; dw < cnt_dw; dw++ )
    {
        *((U32 *)dst_addr +dw) = val;
    }
}

/*
Function : NFC_OtfbPGReadCMD
Input : U8 ucppu   PU number
        ST_FLASH_CMD_PARAM* pFlashCmdParam   Pointer to flash command parameters
Output : BOOL   TRUE for success ,FALSE for failed
Description :
    1.Get otfb addr
    2.Read from flash to otfb
    3.Get sgq
    4.Xfer data from otfb to host
*/
BOOL NFC_OtfbPGReadCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param)
{
    FLASH_PHY flash_phy;
    SIM_NFC_RED *pLocalRed;
    U8 pg_type;
    U8 bSecHighDW,bitPosInDW;
    U32 dwToOtfb;
    U32 secIndex;
    U32 ulSecCnt;
    U32* p_buf;
    U32* p_addr = NULL;
    U32* p_addr_red = NULL;
    U16 phyPageIndex;
    U8  rp;
    U8 usErrCode = NF_SUCCESS;
    U32 bCheckNfcErr = TRUE;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);

    //U32 ulBmid;
    rp = NFC_GetRP(tNfcOrgStruct);
    //read whole logic-page out with redundant
    pLocalRed = (SIM_NFC_RED *)pFlashCmdParam->p_local_red;
    p_buf = g_aDataBufferOut[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu];
#ifdef SIM_XTENSA
    nType = RAND_DATA_TYPE;
#endif
    //for (phyPageIndex = 0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
    for (phyPageIndex = 0; phyPageIndex < pFlashCmdParam->bsPlnNum; phyPageIndex++)
    {
        pFlashCmdParam->phy_page_req[phyPageIndex].row_addr = pFlashCmdParam->p_nf_cq_entry->atRowAddr[phyPageIndex].bsRowAddr;
        pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl = NFC_GetPartInWL(pFlashCmdParam->bsCmdCode);

        NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->phy_page_req[phyPageIndex].row_addr,
            pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl, pFlashCmdParam->bsIsTlcMode, &flash_phy);
        p_addr_red = &pLocalRed->m_Content[g_RED_SZ_DW*phyPageIndex];
#ifdef SIM_XTENSA
        Mid_Read(&flash_phy, nType, (char*) p_addr_red, g_RED_SZ_DW*sizeof(U32));
#else
        usErrCode = Mid_Read(&flash_phy, (char*)p_buf + phyPageIndex*g_PG_SZ, (char*)p_addr_red, g_PG_SZ, bCheckNfcErr, pFlashCmdParam, tNfcOrgStruct, (phyPageIndex / PHY_PAGE_PER_LOGIC_PAGE));

        if (TRUE == bCheckNfcErr && NF_SUCCESS != usErrCode)
        {
            NFC_ErrInj(usErrCode, pFlashCmdParam);
        }

        bCheckNfcErr = TRUE;
#endif
        /*record command*/
        FlashStsM_SetPreCmdInfo(&flash_phy, pFlashCmdParam);
    }

    //judge data type
    pg_type = Flash_Talbe_GetPageType(&flash_phy);
    dwToOtfb = (pg_type == COM_DATA_TYPE) ? 2 : SEC_SIZE / sizeof(U32);

    //copy data to DMA_buffer
    ulSecCnt = 0;

    //FIRMWARE_LogInfo("NFCOtfbRead PU %d PhyBlk %d Pg %d PgType %d pLocalRed 0x%x \n", flash_phy.ucLunInTotal, flash_phy.nBlock, flash_phy.nPage, pg_type, pLocalRed);

#if (TRUE == SGE_ENABLE)
    p_addr = (U32*)SGE_GetOtfbAddr(ucPhyPuInTotal);
#endif

    for (phyPageIndex = 0; phyPageIndex < pFlashCmdParam->bsPlnNum; phyPageIndex++)
    {
        if((pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[0] != 0
            || pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[1] != 0)
            && pFlashCmdParam->red_only != 1)
        {
            p_buf = g_aDataBufferOut[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu] + phyPageIndex*g_PG_SZ / sizeof(U32);
        #ifdef SIM_XTENSA
            NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->phy_page_req[phyPageIndex].row_addr,
                pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl, pFlashCmdParam->bsIsTlcMode, &flash_phy);
            Mid_Read(&flash_phy, nType, (char*) p_buf, g_PG_SZ);
        #endif
            secIndex = 0;
            while(secIndex<64)
            {
                bSecHighDW = (secIndex>31)? 1 : 0;
                bitPosInDW = (bSecHighDW==1)?(secIndex-32):secIndex;
                if(pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[bSecHighDW] & (1<<bitPosInDW))
                {
                    Comm_WriteOtfb((U32)(p_addr + (SEC_SIZE*ulSecCnt) / sizeof(U32)), dwToOtfb, p_buf + (SEC_SIZE*secIndex) / sizeof(U32));

                    ulSecCnt++;
                }
                secIndex++;
            }
        }
    }

#if (TRUE == SGE_ENABLE)
    SGE_OtfbToHost(ucPhyPuInTotal, tNfcOrgStruct->ucLunInPhyPu, rp, pFlashCmdParam->module_inj_type);
#endif

    return TRUE;
}

BOOL NFC_OtfbPGWriteCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam)
{
    FLASH_PHY flash_phy;
    SIM_NFC_RED *pLocalRed;
    U8 pg_type;
    U8 bSecHighDW,bitPosInDW;
    int nType;
    U32  ulDLenOtfb;
    U32 secIndex;
    U32* p_buf;
    //U32* p_buf_temp;
    U32* p_addr = NULL;
    U32* p_addr_red = NULL;
    U16 phyPageIndex;
    BOOL bNcq1stEn = FALSE;
    U32 ulRedOPType = INVALID_8F;

    U32 ulBmid = 0;
    U8 ucRp = NFC_GetRP(tNfcOrgStruct);
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);

    //copy redundant data to local memory
    pLocalRed = (SIM_NFC_RED*)pFlashCmdParam->p_local_red;
    Comm_ReadOtfb(pFlashCmdParam->p_red_addr, sizeof(SIM_NFC_RED) / sizeof(U32), (U32 *)pLocalRed);

    ulRedOPType = NFC_GetSpareOpType((U32 *)pLocalRed);
    pg_type = NFC_GetSparePGType((SIM_NFC_RED *)pLocalRed);

    if (PG_TYPE_FREE != pg_type)
    {
        if (PG_TYPE_DATA_SIMNFC == pg_type)//A : write whole page
        {
            nType = COM_DATA_TYPE;
            ulDLenOtfb = 2;
        }
        else//B : write compressed data : 2 DW for 1 sector
        {
            nType = RSV_DATA_TYPE;
            ulDLenOtfb = SEC_SIZE / sizeof(U32);
        }

        p_buf = g_pDataBufferIn;
#if (TRUE == SGE_ENABLE)
        SGE_HostToOtfb(ucPhyPuInTotal, tNfcOrgStruct->ucLunInPhyPu ,ucRp);
        p_addr = (U32*)SGE_GetOtfbAddr(ucPhyPuInTotal);
#endif
        for (phyPageIndex = 0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
        {
            if (pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[0] != 0 ||
                pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[1] != 0)
            {
                NFC_GetFlashAddr(tNfcOrgStruct, pFlashCmdParam->phy_page_req[phyPageIndex].row_addr,
                    pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl, pFlashCmdParam->bsIsTlcMode, &flash_phy);

                NFC_RecordLastFlashAddr(&flash_phy);

                //write redundant data
                p_addr_red = (U32*)&pLocalRed->m_Content[g_RED_SZ_DW*phyPageIndex];
            #ifdef SIM_XTENSA
                Mid_Write(&flash_phy, RAND_DATA_TYPE, (char*) p_addr_red, g_RED_SZ_DW*sizeof(U32));
            #endif
                secIndex = 0;
                while (secIndex < 64)
                {
                    bSecHighDW = (secIndex > 31) ? 1 : 0;
                    bitPosInDW = (bSecHighDW == 1) ? (secIndex - 32) : secIndex;
                    if (pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[bSecHighDW] & (1 << bitPosInDW))
                    {
                        Comm_ReadOtfb((U32)(p_addr + (phyPageIndex*g_PG_SZ) / sizeof(U32) + (SEC_SIZE*secIndex) / sizeof(U32)),
                                      ulDLenOtfb,
                            p_buf + (SEC_SIZE*secIndex) / sizeof(U32));

                         //check lba in buffer with LPN in spare
                        if (COM_DATA_TYPE == nType)
                        {
                            #ifdef SIM
                            if (FALSE == NFC_CheckData(p_buf + (SEC_SIZE*secIndex) / sizeof(U32), pLocalRed->m_DataRed.aCurrLPN[secIndex / SEC_PER_LPN],
                                secIndex % SEC_PER_LPN, ulRedOPType))
                            #else
                            if (FALSE == NFC_CheckData(p_buf+(SEC_SIZE*secIndex)/sizeof(U32),pLocalRed->m_DataRed.aCurrLPN[secIndex/SEC_PER_LPN],secIndex % SEC_PER_LPN, ulRedOPType))
                            #endif
                            {
                                DBG_Printf("NFC_PGWriteCMD: check Data Error!\n");
                                DBG_Break();
                            }
                        }
                    }

                    secIndex++;
                }
                #ifdef SIM_XTENSA
                Mid_Write(&flash_phy, nType, (char*) p_buf, g_PG_SZ);
                #else
                Mid_Write(&flash_phy, nType, (char*)p_buf, (char *)p_addr_red, g_PG_SZ, TRUE, pFlashCmdParam, tNfcOrgStruct, pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl);
                #endif
            }
            p_buf += g_PG_SZ / sizeof(U32);
        }
    }
    else
    {
        DBG_Printf("Function:NFC_PGWriteCMD-- invalid page type(PG_TYPE_FREE)\n");
        DBG_Getch();
    }

    return TRUE;
}

BOOL NFC_IsAddrUECC(FLASH_PHY* pFlash_phy)
{
    U32 nLoop;
    BOOL bRtn = FALSE;

    for (nLoop = 0; nLoop < ERR_INJ_TABLE_MAX; nLoop++)
    {
        if (!g_stFlashErrInjEntry[nLoop].valid)
            continue;
        else
        {
            if ((NF_ERR_TYPE_UECC == g_stFlashErrInjEntry[nLoop].err_type) &&
                (pFlash_phy->bsPhyPuInTotal == g_stFlashErrInjEntry[nLoop].ucPhyPuInTotal) &&
                (pFlash_phy->nBlock == g_stFlashErrInjEntry[nLoop].block) &&
                (pFlash_phy->nPage == g_stFlashErrInjEntry[nLoop].page))
            {
                if (g_stFlashErrInjEntry[nLoop].retry_times > HAL_FLASH_READRETRY_CNT)
                {
                    bRtn = TRUE;
                    break;
                }
            }
        }
    }
    if (FALSE == bRtn)
    {
        if (NF_ERR_TYPE_UECC == Mid_GetFlashErrCode(pFlash_phy))
        {
            bRtn = TRUE;
        }

    }

    return bRtn;
}

/*****************************************************************************
 Prototype      : NFC_GetPendingLPN
 Description    : Get CQ Entry Write Pending LPN
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
#ifndef SIM_XTENSA
void NFC_GetPendingLPN(U32* pPendingLPN,U32* pLPNCnt)
{
#ifdef VT3533_DRIVER_TEST
#else
    U32 nPu = 0;
    U32 i = 0;
    U32 nPendingLpn = 0;
    ST_FLASH_CMD_PARAM flash_param[2*SUBSYSTEM_LUN_MAX];
    ST_FLASH_CMD_PARAM* p_flash_param;
    SIM_NFC_RED RedEntry;
    U8 PageType = 0;
    U8 rp;
    U8 cmd_code;
    NFCQ_ENTRY * p_cq_entry;
    U8 ucNFCmdType;
    U8 ucSubSystemPu;
    U32 ulRedAddr = 0;

    *pLPNCnt = 0;
    for (ucSubSystemPu=0; ucSubSystemPu< SUBSYSTEM_PU_NUM; ucSubSystemPu++)
    {
        NFCM_LUN_LOCATION tNfcOrgStruct = {0};

        nPu = HAL_NfcGetPhyPU(ucSubSystemPu);
        p_flash_param = &flash_param[nPu];

        tNfcOrgStruct.ucCh = NFC_CH_NUM(nPu);
        tNfcOrgStruct.ucPhyPuInCh = NFC_PU_IN_CH(nPu);
        tNfcOrgStruct.ucLunInPhyPu = 0;

        while(FALSE == NFC_GetEmpty(&tNfcOrgStruct))
        {
            rp = NFC_GetRP(&tNfcOrgStruct);
            //p_cq_entry = p_flash_param->p_nf_cq_entry;
            p_cq_entry = COM_GetNFCQEntry(&tNfcOrgStruct, rp);
            cmd_code = NFC_GetCmdCode(&tNfcOrgStruct, rp);
            ucNFCmdType = NF_GetCmdType(cmd_code);
            ulRedAddr = p_cq_entry->bsRedAddr << 4;
            if(CMD_CODE_PROGRAM != ucNFCmdType)
            {
                NFC_InterfaceJumpCQRP(&tNfcOrgStruct);
                continue;
            }
            memset(&RedEntry,0xFF,sizeof(NFC_RED));
            Comm_ReadOtfb(ulRedAddr, sizeof(NFC_RED)/sizeof(U32), (U32*)&RedEntry);
            PageType = (U8)RedEntry.m_RedComm.bcPageType;

            if(PAGE_TYPE_PMT == PageType)
            {
                U32 ulPMTIndex = RedEntry.m_PMTIndex;
                DBG_Printf("MCU %d NFC Pending PMTI %d\n",HAL_GetMcuId(),ulPMTIndex);
            }
            else if(PG_TYPE_DATA_SIMNFC == PageType)
            {
                for(i=0; i<LPN_PER_BUF;i++)
                {
                    nPendingLpn = RedEntry.m_DataRed.aCurrLPN[i];
                    if(nPendingLpn != INVALID_8F)
                    {
                        DBG_Printf("MCU %d Pu %d NFC Pending LPN 0x%x TS 0x%x VirBlk 0x%x\n",HAL_GetMcuId(),nPu,nPendingLpn,RedEntry.m_RedComm.ulTimeStamp,
                            RedEntry.m_RedComm.bsVirBlockAddr);
                        *pPendingLPN = nPendingLpn;
                        pPendingLPN++;
                        (*pLPNCnt)++;
                    }
                }
            }
            NFC_InterfaceJumpCQRP(&tNfcOrgStruct);
        }
        p_flash_param = NULL;
    }
#endif
}
#endif

void NFC_RecordLastFlashAddr(FLASH_PHY *pFlashAddr)
{
    g_LastHandleAddr[pFlashAddr->ucLunInTotal].u64Addr = pFlashAddr->u64Addr;
}

void NFC_InitLastFlashAddr()
{
    memset(g_LastHandleAddr, 0xFF, sizeof(g_LastHandleAddr[NFC_MODEL_LUN_SUM]));
}

BOOL NFC_ReadFlashIDProcess(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    U8 ucLogicPu = p_flash_pucr_reg->aNfcLogicPUReg[tNfcOrgStruct->ucCh][tNfcOrgStruct->ucPhyPuInCh].bsLogicPU;
    volatile FLASH_ID_ENTRY *pFlashID = &(g_pModelNfcDecSts->aFlashId[ucLogicPu][tNfcOrgStruct->ucLunInPhyPu][NFC_GetRP(tNfcOrgStruct)][0]);

    ///@todo Need to replace flash ID value by flash type.
    pFlashID->ulFlashId0 = 0x9394DE98;
    pFlashID->ulFlashId1 = 0x04085076;

    // Initialize DEC Status model
    DecStsM_ReadID();

    return TRUE;
}

/********************** FILE END ***************/

