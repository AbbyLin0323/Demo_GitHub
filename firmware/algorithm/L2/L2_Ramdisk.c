/****************************************************************************
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
*****************************************************************************
Filename     : L2_Ramdisk.c
Version      :   Ver 1.0
Date         :
Author       :  Blakezhang

Description:

Modification History:
20150302     BlakeZhang     001 first create
****************************************************************************/

#include "BaseDef.h"
#include "FW_BufAddr.h"
#include "COM_Memory.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "L2_Ramdisk.h"

#ifdef L2_FAKE

GLOBAL  SCMD   g_L2RamdiskSCMD;
GLOBAL  SUBCMD g_L2RamdiskSubCmd;
GLOBAL  SUBCMD *g_pL2RamdiskSubCmd;
GLOBAL  U32 g_ulL2RamdiskDRAMBase;
GLOBAL  U8  g_ucCurSchedulePu;

extern BOOL L1_HostIO(SUBCMD* pSubCmd);

void L2_RamdiskInit(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    g_ulL2RamdiskDRAMBase = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, L2_RAMDISK_SIZE);

    *pFreeDramBase = ulFreeDramBase;

    // initialize g_ucCurSchedulePu, which is used by fake L2 to
    // indicate which PU we're going to fetch buffer request
    g_ucCurSchedulePu = 0;
    g_pL2RamdiskSubCmd = NULL;

    DBG_Printf("MCU %d L2RamdiskDRAMBase = 0x%x\n", HAL_GetMcuId(), g_ulL2RamdiskDRAMBase);

    return;
}

/****************************************************************************
Name:    L2_RamdiskSectorRW
Input:    Physical Buffer ID, Sector Offset inside Buffer Page, R/W Direction, Target LBA inside
Virtual Storage
Output:    None
Author:    BlakeZhang
Date:    2012.08.30
Description: Copy a sector of data from specified buffer/virtual storage to virtual storage/buffer
Others:
Modify:
****************************************************************************/
void L2_RamdiskSectorRW(U32 ulBufferID, U8 ucSecOffset, U8 bRWDir, U32 ulVSLBA)
{
    U32 ulBufferMemAddr, ulVSMemAddr;
    const U32 ulDWperSector = SEC_SIZE;

    /* 1. Calculate starting address on buffer side */
    ulBufferMemAddr = COM_GetMemAddrByBufferID(ulBufferID, TRUE, BUF_SIZE_BITS)
        + (ucSecOffset << SEC_SIZE_BITS);

    /* 2. Calculate starting address on memory storage */
    ulVSMemAddr = g_ulL2RamdiskDRAMBase + (ulVSLBA << SEC_SIZE_BITS);

    /* 3. Copy data */
    if (DM_READ == bRWDir)
    {
        HAL_DMAECopyOneBlock(ulBufferMemAddr, ulVSMemAddr, ulDWperSector);
    }
    else
    {
        HAL_DMAECopyOneBlock(ulVSMemAddr, ulBufferMemAddr, ulDWperSector);
    }

    return;
}

#ifndef HOST_SATA
void L2_RamdiskBuildSubCmd(BUF_REQ_READ *pSrcBufReq, SUBCMD *pTgtSubCmd)
{
    pTgtSubCmd->pSCMD->ucSCmdType = SCMD_DIRECT_MEDIA_ACCESS;
    pTgtSubCmd->pSCMD->ucSlotNum = pSrcBufReq->tBufReqHostInfo.HmemDptr.bsCmdTag;
    pTgtSubCmd->pSCMD->tMA.ulSubSysLBA = pSrcBufReq->tBufReqHostInfo.HmemDptr.ulLBA;
    pTgtSubCmd->pSCMD->tMA.ucSecLen = pSrcBufReq->ucReqLength;
    pTgtSubCmd->pSCMD->tMA.ucOpType = DM_READ;
    pTgtSubCmd->SubCmdHostInfo.PrdIndex = pSrcBufReq->tBufReqHostInfo.HmemDptr.bsPrdOrPrpIndex;
    pTgtSubCmd->SubCmdHostInfo.PrdOffsetByte = pSrcBufReq->tBufReqHostInfo.HmemDptr.bsOffset;

    pTgtSubCmd->SubCmdHostInfo.HSGBuildByte = 0;
    pTgtSubCmd->SubCmdHostInfo.HSGRemByte = (pTgtSubCmd->pSCMD->tMA.ucSecLen << SEC_SIZE_BITS);
    pTgtSubCmd->SubCmdHostInfo.FirstDSGId = INVALID_4F;
    pTgtSubCmd->SubCmdHostInfo.FirstHSGId = INVALID_4F;
    pTgtSubCmd->SubCmdHostInfo.PreHSGId = INVALID_4F;
    pTgtSubCmd->SubCmdHostInfo.DSGNon = TRUE;
    pTgtSubCmd->SubCmdHostInfo.HSGNon = TRUE;
    pTgtSubCmd->SubCmdHostInfo.QBuilt = FALSE;
    pTgtSubCmd->SubCmdHostInfo.DSGFinish = FALSE;
    pTgtSubCmd->SubCmdHostInfo.HSGFinish = FALSE;
    pTgtSubCmd->SubCmdHostInfo.LenAdd = FALSE;

    pTgtSubCmd->SubCmdStage = SUBCMD_STAGE_IDLE; /* mark for L2 Ramdisk SubCmd */
    pTgtSubCmd->SubCmdHitResult = L1_CACHE_SE_FULL_HIT;
    pTgtSubCmd->SubCmdAddInfo.ucSyncFlag = TRUE;

    return;
}
#endif

BOOL L2_RamdiskIsEnable(U8 ucStage)
{
    if (SUBCMD_STAGE_IDLE == ucStage)
    {
        return TRUE;
    }

    return FALSE;
}

U32 L2_RamdiskGetDramAddr(U32 ulSubSysLBA)
{
    return (HALM_GET_DRAM_ADDR(g_ulL2RamdiskDRAMBase + (ulSubSysLBA << SEC_SIZE_BITS)));
}

void L2_RamdiskTrimProcess(U32 ulStartLPN, U32 ulLPNCount)
{
    U32 ulTrimValue;
    U32 ulVSMemAddr;

    if (ulLPNCount == 0)
    {
        return;
    }

#ifdef SIM
    ulTrimValue = INVALID_8F;
#else
    ulTrimValue = 0;
#endif

    /* 1. Calculate starting address on memory storage */
    ulVSMemAddr = g_ulL2RamdiskDRAMBase + (ulStartLPN << LPN_SIZE_BITS);

    /* 2. Invalidate Trim data */
    HAL_DMAESetValue(ulVSMemAddr, (ulLPNCount*LPN_SIZE), ulTrimValue);

    return;
}

U32 L2_RamdiskEventHandle(void)
{
    COMMON_EVENT L2_Event;
    COMMON_EVENT L3_Event;
    COMM_EVENT_PARAMETER *pParameter;

    L2_Event.Event = 0;
    L3_Event.Event = 0;

    /* check L2 and L3 event, clear event params to success */
    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event))
    {
        if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == CommCheckEvent(COMM_EVENT_OWNER_L3, &L3_Event))
        {
            return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
        else
        {
            CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
            pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
    }
    else
    {
        CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
        pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    /* check L2 event */
    if (L2_Event.EventDbg)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_DBG);
    }
    else if (L2_Event.EventErrorHandling)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_ERR);
    }
    else if (L2_Event.EventLLF)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_LLF);
    }
    else if (L2_Event.EventShutDown)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SHUTDOWN);
    }
    else if (L2_Event.EventBoot)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_BOOT);
    }
    else if (L2_Event.EventRebuild)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD);
    }
    else if (L2_Event.EventIdle)
    {
        g_pMCU12MiscInfo->bSubSystemIdle = TRUE;
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_IDLE);
    }
    else if (L2_Event.EventSaveRT)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVERT);
    }
    else if (L2_Event.EventSelfTest)
    {
        g_pMCU12MiscInfo->bSubSystemSelfTestDone = TRUE;
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SELFTEST);
    }
    else if (L2_Event.EventRebuildDirtyCnt)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD_DIRTYCNT);
    }
    else if (L2_Event.EventBootAfterLoadRT)
    {
#ifndef LCT_VALID_REMOVED
        L1_SetLCTValidLoadStatus(1);
#endif
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_BOOT_AFTER_RT);
    }

    /* check L3 event */
    if (L3_Event.EventDbg)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_DBG);
    }
    else if (L3_Event.EventErrorHandling)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_ERR);
    }
    else if (L3_Event.EventLLF)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_LLF);
    }
    else if (L3_Event.EventShutDown)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_SHUTDOWN);
    }
    else if (L3_Event.EventBoot)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_BOOT);
    }
    else if (L3_Event.EventRebuild)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_REBUILD);
    }
    else if (L3_Event.EventIdle)
    {
        g_pMCU12MiscInfo->bSubSystemIdle = TRUE;
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_IDLE);
    }
    else if (L3_Event.EventSelfTest)
    {
        g_pMCU12MiscInfo->bSubSystemSelfTestDone = TRUE;
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_SELFTEST);
    }

    return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
}

U8 L2_RamdiskSelectReadBufReq(U8 ucSuperPu)
{
    U8 ucCurBufReqID;

    ucCurBufReqID = L1_GetHighPrioLinkHead(ucSuperPu);
    L1_RemoveBufReqFromHighPrioLink(ucSuperPu, INVALID_2F);
   
    return ucCurBufReqID;
}

/****************************************************************************
Name:    L2_RamdiskSchedule
Input:    None
Output:    None
Author:    Blake Zhang
Date:    2012.08.30
Description: The main interface of L2 Ramdisk
Others:
Modify:
****************************************************************************/
void L2_RamdiskSchedule(void)
{
    BUF_REQ_READ *pCurReadBufReq = NULL;
    BUF_REQ_WRITE *pCurWriteBufReq = NULL;
    U8 i;
    U8 ucWorkingPu, ucCurBufReqID;
    BOOL bBuffReqPending;

    U32 ulRequestLBA, ulBuffID;
    U8 ucSectorOffset, ucSectorCount, ucRWDir, ucLPNSectorMap;
    U8 ucCurrLPN, ucLPNCount;
    U8 ucLpnIndex;

#ifdef HOST_SATA
    U8 ucLPNOffset, ucBMID;
    U32 ulLPNMap, ulBMValue;
#endif

    if (COMM_EVENT_STATUS_BLOCKING == L2_RamdiskEventHandle())
    {
        return;
    }

#ifndef HOST_SATA
    /* process pending Virtual SubCmd, AHCI and NVMe mode Host Read only */
    if (NULL != g_pL2RamdiskSubCmd)
    {
        if (TRUE == L1_HostIO(g_pL2RamdiskSubCmd))
        {
            g_pL2RamdiskSubCmd = NULL;
        }

        return;
    }
#endif

    // initialize bBuffReqPending as FALSE
    bBuffReqPending = FALSE;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (TRUE == L1_IsReadBufReqPending(g_ucCurSchedulePu))
        {
            // there is a pending buffer request in the current pu, pop the buffer request
            ucCurBufReqID = L2_RamdiskSelectReadBufReq(g_ucCurSchedulePu);
            pCurReadBufReq = L1_GetReadBufReq(g_ucCurSchedulePu, ucCurBufReqID);

            // record the current pu
            ucWorkingPu = g_ucCurSchedulePu;

            // advance g_ucCurSchedulePu
            g_ucCurSchedulePu = (g_ucCurSchedulePu == (SUBSYSTEM_SUPERPU_NUM - 1)) ? 0 : (g_ucCurSchedulePu + 1);

            // set bBuffReqPending as TRUE
            bBuffReqPending = TRUE;

            // a buffer request has been found, break the search loop
            break;
        }
        else if (TRUE == L1_IsWriteBufReqPending(g_ucCurSchedulePu))
        {
            // there is a pending buffer request in the current pu, pop the buffer request
            pCurWriteBufReq = L1_WriteBufReqPop(g_ucCurSchedulePu);

            // record the current pu
            ucWorkingPu = g_ucCurSchedulePu;

            // advance g_ucCurSchedulePu
            g_ucCurSchedulePu = (g_ucCurSchedulePu == (SUBSYSTEM_SUPERPU_NUM - 1)) ? 0 : (g_ucCurSchedulePu + 1);

            // set bBuffReqPending as TRUE
            bBuffReqPending = TRUE;

            // a buffer request has been found, break the search loop
            break;
        }
        else
        {
            // there aren't any pending buffer request in g_ucCurSchedulePu, advance g_ucCurSchedulePu
            g_ucCurSchedulePu = (g_ucCurSchedulePu == (SUBSYSTEM_SUPERPU_NUM - 1)) ? 0 : (g_ucCurSchedulePu + 1);
        }
    } // for(i = 0; i < PU_NUM; i++)

    /* End process if there is nothing to do */
    if (FALSE == bBuffReqPending)
    {
        return;
    }

    /* 2. Decomposing current buffer request and perform appropriate data access */
    if (NULL != pCurReadBufReq)
    {
        L1_RecycleReadBufReq(ucWorkingPu, ucCurBufReqID);
    }
    else if(NULL != pCurWriteBufReq)
    {
        L1_WriteBufReqMoveHeadPtr(ucWorkingPu);
    }

    /* 4) For a sequential reading/writing, we need only to simply complete the data copy.
        *  Note: "Tag = L1_IDLE_REFETCH_ID" means L1 read-prefech */
    if ((NULL != pCurReadBufReq) && (TRUE == pCurReadBufReq->bReadPreFetch))
    {
        ulBuffID = pCurReadBufReq->usPhyBufferID;
        ucRWDir = L1_REQ_REQTYPE_READ;

        /* Calculate starting LBA of the request */
        ucSectorOffset = pCurReadBufReq->ucReqOffset;
        ulRequestLBA = (pCurReadBufReq->aLPN[0] << LPN_SECTOR_BIT) + ucSectorOffset;

        /* Acquire the transfer length */
        ucSectorCount = pCurReadBufReq->ucReqLength;

        /*************************************
            Execute data copy:
            for sector = 0, counter-1
            begin
            Buffer[offset+sector] = VS_LinearLBA[StartLBA+sector]
            end
            **************************************/
        do
        {
            L2_RamdiskSectorRW(ulBuffID, ucSectorOffset, ucRWDir, ulRequestLBA);
            ucSectorOffset++;
            ulRequestLBA++;
            ucSectorCount--;

        } while (0 < ucSectorCount);

        /* Read PreFetch or AHCI host read through DRAM */
        for (ucLpnIndex = 0; ucLpnIndex < pCurReadBufReq->ucLPNCount; ucLpnIndex++)
        {
            L1_L2SubPrefetchCacheStatus(ulBuffID);
        }
    }
    /* 5) For a cache merge reading, Wbuf or Rbuf,there's only one LPN that shall be processed. But the data copy shall refer to sector map. */
    else if ((NULL != pCurReadBufReq) && (TRUE == pCurReadBufReq->bReqLocalREQFlag))
    {
        ulBuffID = pCurReadBufReq->usPhyBufferID;
        ucRWDir = L1_REQ_REQTYPE_READ;

        /* Calculate start LBA and buffer offset */
        ucCurrLPN = pCurReadBufReq->ucLPNOffset;
        ulRequestLBA = (pCurReadBufReq->aLPN[ucCurrLPN] << LPN_SECTOR_BIT);
        ucSectorOffset = (ucCurrLPN << LPN_SECTOR_BIT);

        /* Scan the sector enable bit map */
        ucLPNSectorMap = pCurReadBufReq->ulLPNBitMap;
        ucSectorCount = LPN_SECTOR_NUM;

        /* Copy each sector that was enabled in specified LPN */
        do
        {
            /* Only copy the enabled sectors */
            if (TRUE == (ucLPNSectorMap & TRUE))
            {
                L2_RamdiskSectorRW(ulBuffID, ucSectorOffset, ucRWDir, ulRequestLBA);
            }

            ucLPNSectorMap >>= 1;
            ulRequestLBA++;
            ucSectorOffset++;
            ucSectorCount--;

        } while (0 < ucSectorCount);

        /* We must clear the flash busy status as well when data copy is finished. */
        if (TRUE == pCurReadBufReq->bReqLocalREQFlag)
        {
            *(U8 *)(pCurReadBufReq->ulReqStatusAddr) = SUBSYSTEM_STATUS_SUCCESS;
        }
    }
    /*host read*/
    else if ((NULL != pCurReadBufReq) && (FALSE == pCurReadBufReq->bReqLocalREQFlag))
    {
        ulBuffID = pCurReadBufReq->usPhyBufferID;
        ucRWDir = L1_REQ_REQTYPE_READ;

#ifdef HOST_SATA
        /* Calculate starting LBA of the request */
        ucSectorOffset = pCurReadBufReq->ucReqOffset;
        ucCurrLPN = pCurReadBufReq->ucLPNOffset;
        ulRequestLBA = (pCurReadBufReq->aLPN[ucCurrLPN] << LPN_SECTOR_BIT) + (ucSectorOffset & SEC_PER_LPN_MSK);

        /* Acquire the transfer length */
        ucSectorCount = pCurReadBufReq->ucReqLength;

        /*************************************
        Execute data copy:
        for sector = 0, counter-1
        begin
        Buffer[offset+sector] = VS_LinearLBA[StartLBA+sector]
        end
        **************************************/
        do {
            L2_RamdiskSectorRW(ulBuffID, ucSectorOffset, ucRWDir, ulRequestLBA);
            ucSectorOffset++;
            ulRequestLBA++;
            ucSectorCount--;
        } while (0 < ucSectorCount);

        /* Calculate buffer map bits */
        ucLPNCount = pCurReadBufReq->ucLPNCount;
        ucLPNOffset = pCurReadBufReq->ucLPNOffset;
        ulLPNMap = (1 << ucLPNCount) - 1;
        ulBMValue = (ulLPNMap << ucLPNOffset);

        /* Program buffer map register */
        ucBMID = pCurReadBufReq->tBufReqHostInfo.ucDSGId;
        HAL_SetBufMapInitValue(ucBMID, ulBMValue);

        /* For the first sub-command within a host command, we also need to
            assert the first read data ready flag of corresponding command slot. */
        if (TRUE == pCurReadBufReq->bFirstRCMDEn)
        {
            HAL_SetFirstReadDataReady(pCurReadBufReq->tBufReqHostInfo.ucTag);
        }
#else
        g_pL2RamdiskSubCmd = &g_L2RamdiskSubCmd;
        g_pL2RamdiskSubCmd->pSCMD = &g_L2RamdiskSCMD;

        L2_RamdiskBuildSubCmd(pCurReadBufReq, g_pL2RamdiskSubCmd);

        if (TRUE == L1_HostIO(g_pL2RamdiskSubCmd))
        {
            g_pL2RamdiskSubCmd = NULL;
        }
#endif
    }
    /* 6) For a cache flush request, all four LPNs may not be in sequential order,
        but the buffer is always full */
    else if (NULL != pCurWriteBufReq)
    {
        ulBuffID = pCurWriteBufReq->PhyBufferID;
        ucRWDir = L1_REQ_REQTYPE_WRITE;

        /* Acquire data start LPN and valid LPN count in current buffer */
        ucCurrLPN = 0;
        ucLPNCount = LPN_PER_BUF;

        /* Copy all valid LPNs */
        do {
            /* For each valid LPN: */
            if (INVALID_8F != pCurWriteBufReq->LPN[ucCurrLPN]) {
                /* Calculate linear block address and sector offset in the buffer */
                ulRequestLBA = (pCurWriteBufReq->LPN[ucCurrLPN] << LPN_SECTOR_BIT);
                ucSectorOffset = (ucCurrLPN << LPN_SECTOR_BIT);
                ucSectorCount = LPN_SECTOR_NUM;

                //L1_DbgTrans(0, ucRWDir, ulRequestLBA, ucSectorCount);
                /* Copy all 8 sectors inside one LPN */
                do {
                    /* For each sector: */
                    L2_RamdiskSectorRW(ulBuffID, ucSectorOffset, ucRWDir, ulRequestLBA);

                    ulRequestLBA++;
                    ucSectorOffset++;
                    ucSectorCount--;
                } while (0 < ucSectorCount);
            }

            ucCurrLPN++;
            ucLPNCount--;
        } while (0 < ucLPNCount);

        /* We need to clear cache status for a cache flush request. */
        *(U8 *)(pCurWriteBufReq->ReqStatusAddr) = SUBSYSTEM_STATUS_SUCCESS;
    }
    else
    {
        DBG_Printf("L2_Ramdisk ERROR BufReq pReadBufReq = 0x%x, pWriteBufReq = 0x%x\n", (U32)pCurReadBufReq, (U32)pCurWriteBufReq);
        DBG_Getch();
    }

    return;
}

#endif

/********************** FILE END ***************/

