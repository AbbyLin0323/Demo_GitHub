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
Filename     : L1_NvmeIO.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  NinaYang

Description: 

Modification History:
20141124 NinaYang 001 first create
*****************************************************************************/
#include "HAL_Inc.h"
#include "HAL_TraceLog.h"
#include "L1_Inc.h"
#include "COM_Memory.h"
#include "COM_QWord.h"
#include "FW_BufAddr.h"

#ifdef L2_FAKE
#include "L2_Ramdisk.h"
#endif

#ifdef DATA_MONITOR_ENABLE
#include "FW_DataMonitor.h"
#endif

#define TL_FILE_NUM L1_NVMeIO_c

extern MCU12_VAR_ATTR CHAIN_NUM_MGR* l_aChainNumMgr;

void MCU1_DRAM_TEXT L1_HostIOInit(void)
{
    HAL_ChainMaintainInit();
    return;
}

void MCU1_DRAM_TEXT L1_HostIOWarmInit(void)
{
    HAL_ChainMaintainInit();
    return;
}

void L1_InitSubCmdHostInfo(SUBCMD* pSubCmd)
{
    //We should move the most unlikely to the 1st.
    // we don't need to process the host io stage if the current write command is write zero
    if((pSubCmd->pSCMD->tMA.ucOption == DM_WRITE_ZERO) && (pSubCmd->pSCMD->ucSCmdType == SCMD_DIRECT_MEDIA_ACCESS) && (pSubCmd->pSCMD->tMA.ucOpType == DM_WRITE))
    {
        // we do not conduct any host IO related operations when processing
        // write zero subcommand
        return;
    }

    /*fill SGEInfo common */
    pSubCmd->SubCmdHostInfo.HSGBuildByte = 0;
    pSubCmd->SubCmdHostInfo.FirstDSGId = INVALID_4F;
    pSubCmd->SubCmdHostInfo.FirstHSGId = INVALID_4F;
    pSubCmd->SubCmdHostInfo.PreHSGId = INVALID_4F;
    pSubCmd->SubCmdHostInfo.DSGNon = TRUE;
    pSubCmd->SubCmdHostInfo.HSGNon = TRUE;
    pSubCmd->SubCmdHostInfo.QBuilt = FALSE;
    pSubCmd->SubCmdHostInfo.DSGFinish = FALSE;
    pSubCmd->SubCmdHostInfo.HSGFinish = FALSE;
    pSubCmd->SubCmdHostInfo.LenAdd = FALSE;

    if ((U8)SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
    {
        /*fill SGEInfo special */
        pSubCmd->SubCmdHostInfo.HSGRemByte =  (pSubCmd->pSCMD->tMA.ucSecLen << SEC_SIZE_BITS);
        pSubCmd->SubCmdHostInfo.PrdIndex = pSubCmd->pSCMD->tMA.usHPRDEntryID;
        pSubCmd->SubCmdHostInfo.PrdOffsetByte = pSubCmd->pSCMD->tMA.ulHPRDMemOffset;

        if ((TRUE == pSubCmd->pSCMD->tMA.ucFirst) && (DM_READ == pSubCmd->pSCMD->tMA.ucOpType))
        {
            l_aChainNumMgr[pSubCmd->pSCMD->ucSlotNum].usTotalSecCnt = pSubCmd->pSCMD->tMA.ulHCmdSecCnt;
        }
    }
    else
    {
        /*fill SGEInfo special */
        pSubCmd->SubCmdHostInfo.HSGRemByte =  (pSubCmd->pSCMD->tRawData.ucSecLen << SEC_SIZE_BITS);
        pSubCmd->SubCmdHostInfo.PrdIndex = 0;
        pSubCmd->SubCmdHostInfo.PrdOffsetByte = 0;
    }

    return;
}

/* MovePrdtPtr when split SubCmd */
BOOL L1_UpdateHostAddrInfo(SCMD *pPreSCmd, SCMD *pTgtSCmd)
{
    U16 usPrpIndex = pPreSCmd->tMA.usHPRDEntryID;
    U32 ulCurPRPOffset = pPreSCmd->tMA.ulHPRDMemOffset;
    U32 ulMoveRemByte = (pPreSCmd->tMA.ucSecLen << SEC_SIZE_BITS);
    U8 ucCmdTag = pTgtSCmd->ucSlotNum;
    U32 ulCurPRPXferBytes = 0;

    if (pPreSCmd->ucSlotNum != pTgtSCmd->ucSlotNum)
    {
        DBG_Printf("L1_MovePrdtPtr() error: pPreSCmd->ucHCmdTag != pTgtSCmd->ucSlotNum\n");
        DBG_Getch();
        return FALSE;
    }

    while (ulMoveRemByte > 0)
    {
        ulCurPRPXferBytes = min((HPAGE_SIZE - ulCurPRPOffset) , ulMoveRemByte);

        if (ulCurPRPXferBytes == ulMoveRemByte)//move complete
        {
            ulCurPRPOffset += ulMoveRemByte;
            if(ulCurPRPOffset >= HPAGE_SIZE)
            {
                usPrpIndex++;
                ulCurPRPOffset = ulCurPRPOffset - HPAGE_SIZE;
            }
            ulMoveRemByte = 0;
        }
        else if (ulCurPRPXferBytes < ulMoveRemByte)
        {
            usPrpIndex++;
            ulCurPRPOffset = 0;
            ulMoveRemByte -= ulCurPRPXferBytes; 

            if (usPrpIndex > (MAX_PRP_NUM - 1))
            {
                DBG_Printf("L1_MovePrdtPtr() error: ucPrpIndex %d\n",usPrpIndex);
                DBG_Getch();
                return FALSE;
            }
        }
    }

    pTgtSCmd->tMA.usHPRDEntryID = usPrpIndex;
    pTgtSCmd->tMA.ulHPRDMemOffset = ulCurPRPOffset;

    return TRUE;
}

BOOL L1_GetHostResource(SUBCMD* pSubCmd)
{
    return TRUE;
}

/****************************************************************************
*Build a HSG
*****************************************************************************/
BOOL L1_NvmeBuildHSG(SUBCMD *pSubCmd)
{
    U8 ucCmdTag;
    U8 ucPrpIndex;
    PPRP pPRP;
    HSG_ENTRY *pCurHSG;
    QWORD tHostAddr;
    QWORD tPRPAddr;
    U32 ulPrpRemByte;
    HSG_ENTRY *pPreHSG;
    U16 usCurHSGId;
    U32 ulLBAOffset;
    U32 ulAddress;

#if 1
    if (FALSE == HAL_GetHsg(&usCurHSGId))
    {
        return FALSE;
    }
    pCurHSG = (HSG_ENTRY *)HAL_GetHsgAddr(usCurHSGId);
    COM_MemZero((U32 *)pCurHSG, sizeof(HSG_ENTRY)/4);

    ucPrpIndex = pSubCmd->SubCmdHostInfo.PrdIndex;
    ucCmdTag   = pSubCmd->pSCMD->ucSlotNum;
    pPRP = (PRP *)HAL_GetLocalPrpEntryAddr(ucCmdTag, ucPrpIndex);
    
    if (SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
    {
        ulAddress  = pSubCmd->pSCMD->tMA.ulSubSysLBA;
    }
    else
    {
        ulAddress  = pSubCmd->pSCMD->tRawData.ulBuffAddr;
        if (0 == ucPrpIndex)
        {
            pSubCmd->SubCmdHostInfo.PrdOffsetByte = pPRP->Offset << 2;
        }
    }

    /*Fill host address*/
    tPRPAddr.LowDw = (pPRP->PBAL << 12);
    tPRPAddr.HighDw = pPRP->PBAH;
    COM_QwAddDw((QWORD *)(&tPRPAddr), pSubCmd->SubCmdHostInfo.PrdOffsetByte, &tHostAddr);
    pCurHSG->ulHostAddrLow  = tHostAddr.LowDw;
    pCurHSG->ulHostAddrHigh = tHostAddr.HighDw;

    /*Fill Data Len*/
    ulPrpRemByte = (HPAGE_SIZE) - pSubCmd->SubCmdHostInfo.PrdOffsetByte;
    pCurHSG->bsLength = (pSubCmd->SubCmdHostInfo.HSGRemByte >= ulPrpRemByte) ? ulPrpRemByte : pSubCmd->SubCmdHostInfo.HSGRemByte;

    // Quick process for single PRP case
    if (pSubCmd->SubCmdHostInfo.HSGNon)/*first HSG*/
    {
        pSubCmd->SubCmdHostInfo.HSGNon = FALSE;
        pSubCmd->SubCmdHostInfo.FirstHSGId = usCurHSGId;

        if (pSubCmd->SubCmdHostInfo.HSGRemByte == pCurHSG->bsLength) {
            pCurHSG->ulLBA = ulAddress; // Due to 'ulLBAOffset' should be 0
            pSubCmd->SubCmdHostInfo.HSGFinish = TRUE;
            pCurHSG->bsLast = TRUE;
            HAL_SetHsgSts(usCurHSGId, 1);

            return TRUE;
        }
    }
    else
    {
        /*finish previous HSG*/
        pPreHSG = (HSG_ENTRY *)HAL_GetHsgAddr(pSubCmd->SubCmdHostInfo.PreHSGId);
        pPreHSG->bsNextHsgId = usCurHSGId;
        HAL_SetHsgSts(pSubCmd->SubCmdHostInfo.PreHSGId, 1);
    }

    /*Fill Data Len*/
    if (0 == pCurHSG->bsLength)
    {
        DBG_Printf("HSG Length is zero!\n");
        DBG_Getch();
    }

    if (0 == (pSubCmd->SubCmdHostInfo.HSGBuildByte & SEC_SIZE_MSK))
    {
        ulLBAOffset = (pSubCmd->SubCmdHostInfo.HSGBuildByte >> SEC_SIZE_BITS);
    }
    else
    {
        ulLBAOffset = (pSubCmd->SubCmdHostInfo.HSGBuildByte >> SEC_SIZE_BITS) + 1;
    }
    pCurHSG->ulLBA = ulAddress + ulLBAOffset;

    /*Update SubCmd Info*/
    pSubCmd->SubCmdHostInfo.HSGBuildByte += pCurHSG->bsLength;
    pSubCmd->SubCmdHostInfo.HSGRemByte   -= pCurHSG->bsLength;

    if (0 == pSubCmd->SubCmdHostInfo.HSGRemByte)/*Last HSG*/
    {
        pSubCmd->SubCmdHostInfo.HSGFinish = TRUE;
        pCurHSG->bsLast = TRUE;
        HAL_SetHsgSts(usCurHSGId, 1);
    }
    else
    {
        pSubCmd->SubCmdHostInfo.PrdIndex++;
        pSubCmd->SubCmdHostInfo.PrdOffsetByte = 0;    
        /*Update g_usPreHSGId*/
        pSubCmd->SubCmdHostInfo.PreHSGId = usCurHSGId;
    }
#else
    ucPrpIndex = pSubCmd->SubCmdHostInfo.PrdIndex;
    ucCmdTag   = pSubCmd->pSCMD->ucSlotNum;
    pPRP = (PRP *)HAL_GetLocalPrpEntryAddr(ucCmdTag, ucPrpIndex);
    
    if (SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
    {
        ulAddress  = pSubCmd->pSCMD->tMA.ulSubSysLBA;
    }
    else
    {
        ulAddress  = pSubCmd->pSCMD->tRawData.ulBuffAddr;
        if(0 == ucPrpIndex)
        {
            pSubCmd->SubCmdHostInfo.PrdOffsetByte = pPRP->Offset << 2;
        }
    }

    if (FALSE == HAL_GetHsg(&usCurHSGId))
    {
        return FALSE;
    }
    pCurHSG = (HSG_ENTRY *)HAL_GetHsgAddr(usCurHSGId);
    COM_MemZero((U32 *)pCurHSG, sizeof(HSG_ENTRY)/4);

    if (pSubCmd->SubCmdHostInfo.HSGNon)/*first HSG*/
    {
        pSubCmd->SubCmdHostInfo.HSGNon = FALSE;
        pSubCmd->SubCmdHostInfo.FirstHSGId = usCurHSGId;
    }
    else
    {
        /*finish previous HSG*/
        pPreHSG = (HSG_ENTRY *)HAL_GetHsgAddr(pSubCmd->SubCmdHostInfo.PreHSGId);
        pPreHSG->bsNextHsgId = usCurHSGId;
        HAL_SetHsgSts(pSubCmd->SubCmdHostInfo.PreHSGId, 1);
    }

    /*Fill host address*/
    tPRPAddr.LowDw = (pPRP->PBAL << 12);
    tPRPAddr.HighDw = pPRP->PBAH;
    COM_QwAddDw((QWORD *)(&tPRPAddr), pSubCmd->SubCmdHostInfo.PrdOffsetByte, &tHostAddr);
    pCurHSG->ulHostAddrLow =  tHostAddr.LowDw;
    pCurHSG->ulHostAddrHigh = tHostAddr.HighDw;

    /*Fill Data Len*/
    ulPrpRemByte = (HPAGE_SIZE) - pSubCmd->SubCmdHostInfo.PrdOffsetByte;
    pCurHSG->bsLength = (pSubCmd->SubCmdHostInfo.HSGRemByte >= ulPrpRemByte) ? ulPrpRemByte : pSubCmd->SubCmdHostInfo.HSGRemByte;
    if (0 == pCurHSG->bsLength)
    {
        DBG_Printf("HSG Length is zero!\n");
        DBG_Getch();
    }

    if (0 == (pSubCmd->SubCmdHostInfo.HSGBuildByte & SEC_SIZE_MSK))
    {
        ulLBAOffset = (pSubCmd->SubCmdHostInfo.HSGBuildByte >> SEC_SIZE_BITS);
    }
    else
    {
        ulLBAOffset = (pSubCmd->SubCmdHostInfo.HSGBuildByte >> SEC_SIZE_BITS) + 1;
    }
    pCurHSG->ulLBA = ulAddress + ulLBAOffset;

    /*Update SubCmd Info*/
    pSubCmd->SubCmdHostInfo.HSGBuildByte += pCurHSG->bsLength;
    pSubCmd->SubCmdHostInfo.HSGRemByte -= pCurHSG->bsLength;

    if (0 == pSubCmd->SubCmdHostInfo.HSGRemByte)/*Last HSG*/
    {
        pSubCmd->SubCmdHostInfo.HSGFinish = TRUE;
        pCurHSG->bsLast = TRUE;
        HAL_SetHsgSts(usCurHSGId, 1);
    }
    else
    {
        pSubCmd->SubCmdHostInfo.PrdIndex++;
        pSubCmd->SubCmdHostInfo.PrdOffsetByte = 0;    
        /*Update g_usPreHSGId*/
        pSubCmd->SubCmdHostInfo.PreHSGId = usCurHSGId;
    }
#endif

    return TRUE;
}

void L1_FillHostAddrInfo(SUBCMD *pSubCmd, BUF_REQ_READ *pBufReq)
{
    SCMD *pSCMD;
    HMEM_DPTR *pHmemDptr;

    pSCMD = pSubCmd->pSCMD;
    pHmemDptr = &(pBufReq->tBufReqHostInfo.HmemDptr);

    /*fill HMEM_DPTR*/
    pHmemDptr->bsCmdTag = pSCMD->ucSlotNum;
    pHmemDptr->ulLBA = pSCMD->tMA.ulSubSysLBA;
    pHmemDptr->bsPrdOrPrpIndex = pSCMD->tMA.usHPRDEntryID;
    pHmemDptr->bsOffset = pSCMD->tMA.ulHPRDMemOffset;

    return;
}


BOOL L1_NvmeBuildDSG(SUBCMD *pSubCmd)
{
    NORMAL_DSG_ENTRY *pCurDSG;
    U16 usCurDSGId;

    if (FALSE == HAL_GetNormalDsg(&usCurDSGId))
    {
        return FALSE;
    }

    pCurDSG = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(usCurDSGId);
    COM_MemZero((U32 *)pCurDSG, sizeof(NORMAL_DSG_ENTRY)/sizeof(U32));

    pSubCmd->SubCmdHostInfo.DSGFinish = TRUE;
    pSubCmd->SubCmdHostInfo.DSGNon = FALSE;
    pSubCmd->SubCmdHostInfo.FirstDSGId = usCurDSGId;

    /*Fill DSG*/
    if ((U8)SCMD_RAW_DATA_REQ == pSubCmd->pSCMD->ucSCmdType)
    {
        pCurDSG->bsDramAddr    = HALM_GET_DRAM_ADDR(pSubCmd->pSCMD->tRawData.ulBuffAddr) >> 1;
        pCurDSG->bsXferByteLen = (pSubCmd->pSCMD->tRawData.ucSecLen << SEC_SIZE_BITS);
        pCurDSG->bsCacheStsEn  = TRUE;
    }
    else
    {
        if (L1_RamdiskIsEnable())
        {
            pCurDSG->bsDramAddr   = L1_RamdiskGetDramAddr(pSubCmd->pSCMD) >> 1;
            pCurDSG->bsCacheStsEn = FALSE;
        }
#ifdef L2_FAKE
        else if (L2_RamdiskIsEnable(pSubCmd->SubCmdStage))
        {
            pCurDSG->bsDramAddr = L2_RamdiskGetDramAddr(pSubCmd->pSCMD->tMA.ulSubSysLBA) >> 1;
            pCurDSG->bsCacheStsEn = FALSE;
        }
#endif
        else
        {
            pCurDSG->bsDramAddr   = ((((U32)pSubCmd->SubCmdPhyBufferID) << BUF_SIZE_BITS)
                                  + (((U32)pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT) << SEC_SIZE_BITS))>>1;

            if (L1_CACHE_SE_RD_INVALID != pSubCmd->SubCmdHitResult)
            {
                pCurDSG->bsCacheStsEn = TRUE;
            }
            else
            {
                pCurDSG->bsCacheStsEn = FALSE;
            }
        }

        pCurDSG->bsXferByteLen = (pSubCmd->pSCMD->tMA.ucSecLen << SEC_SIZE_BITS);
    }

    pCurDSG->bsCacheStatusAddr =  pSubCmd->CacheStatusAddr; 

    pCurDSG->bsLast = TRUE;

    /*set CacheStatus*/
    pCurDSG->bsCacheStsData = SUBSYSTEM_STATUS_SUCCESS;

    HAL_SetNormalDsgSts(usCurDSGId, 1);

    return TRUE;
}

BOOL L1_HostIO(SUBCMD* pSubCmd)
{
    BOOL bRet;
    SCMD *pSCMD;

    bRet  = FALSE;
    pSCMD = pSubCmd->pSCMD;

    /* check for Host read from OTFB case  */
    if (((U8)SCMD_DIRECT_MEDIA_ACCESS == pSCMD->ucSCmdType) && 
        ((U8)DM_READ == pSCMD->tMA.ucOpType) && (L1_CACHE_SE_NO_HIT == pSubCmd->SubCmdHitResult)
#ifdef HOST_READ_FROM_DRAM
        && (TRUE != pSubCmd->SubCmdAddInfo.ucSyncFlag)
#endif
    )
    {
        return TRUE;
    }

    // we don't need to process the host io stage if the current write command is write zero
    // We should move the most unlikely to the 1st.
    if((pSCMD->tMA.ucOption == DM_WRITE_ZERO) && (pSCMD->ucSCmdType == SCMD_DIRECT_MEDIA_ACCESS) && (pSCMD->tMA.ucOpType == DM_WRITE))
    {
        return TRUE;
    }

    if (FALSE == pSubCmd->SubCmdHostInfo.DSGFinish)
    {
        L1_NvmeBuildDSG(pSubCmd);
    }

    TL_PERFORMANCE(PTL_LEVEL_DETAIL,"L1 nvme build DSG");

    while (FALSE == pSubCmd->SubCmdHostInfo.HSGFinish)
    {
        TL_PERFORMANCE(PTL_LEVEL_NULL,"L1 nvme build one DSG");
        if (FALSE == L1_NvmeBuildHSG(pSubCmd))
        {
            break;
        }
    }

    TL_PERFORMANCE(PTL_LEVEL_DETAIL,"L1 nvme build all HSG");

    if (FALSE == pSubCmd->SubCmdHostInfo.LenAdd)
    {
        if ((U8)SCMD_DIRECT_MEDIA_ACCESS == pSCMD->ucSCmdType)
        {
            /*add finished request length and chain num, if equal to total request length, get total chain num*/
            if (pSubCmd->pSCMD->tMA.ucOpType == DM_READ)
            {
                HAL_AddFinishReqLength(pSCMD->ucSlotNum, pSCMD->tMA.ucSecLen);
            }
        }

        pSubCmd->SubCmdHostInfo.LenAdd = TRUE;
    }

    if ((TRUE == pSubCmd->SubCmdHostInfo.DSGFinish) && (TRUE == pSubCmd->SubCmdHostInfo.HSGFinish))
    {
        /* Build DRQ/DWQ */
        if ((FALSE == pSubCmd->SubCmdHostInfo.QBuilt) && (FALSE == pSubCmd->SubCmdHostInfo.HSGNon) && (FALSE == pSubCmd->SubCmdHostInfo.DSGNon))
        {
            if ((((U8)SCMD_DIRECT_MEDIA_ACCESS == pSCMD->ucSCmdType) && ((U8)DM_WRITE == pSCMD->tMA.ucOpType))
                || (((U8)SCMD_RAW_DATA_REQ == pSCMD->ucSCmdType) && ((U8)RAWDRQ_H2D == pSCMD->tRawData.ucDataDir)))
            {
                if (FALSE == HAL_DwqBuildEntry(pSCMD->ucSlotNum, pSubCmd->SubCmdHostInfo.FirstHSGId, pSubCmd->SubCmdHostInfo.FirstDSGId))
                {
                    /* DWQ build fail */
                    return FALSE;
                }
            }
            else
            {
                if (FALSE == HAL_DrqBuildEntry(pSCMD->ucSlotNum, pSubCmd->SubCmdHostInfo.FirstHSGId, pSubCmd->SubCmdHostInfo.FirstDSGId))
                {
                    /* DRQ build fail */
                    return FALSE;
                }
            }
        
            pSubCmd->SubCmdHostInfo.QBuilt = TRUE;
        }    

#ifdef DATA_MONITOR_ENABLE
        if (((U8)SCMD_DIRECT_MEDIA_ACCESS == pSCMD->ucSCmdType) && ((U8)DM_READ == pSCMD->tMA.ucOpType))
        {
            FW_DataMonitorCheckRange(pSCMD->tMA.ulSubSysLBA, pSCMD->tMA.ucSecLen, pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT,
                COM_GetMemAddrByBufferID(pSubCmd->SubCmdPhyBufferID, TRUE, BUF_SIZE_BITS));
        }
#endif

        bRet = TRUE;
    }

    TL_PERFORMANCE(PTL_LEVEL_DETAIL,"L1 nvme build DRDWQ");

    return bRet;
}


/**************** FILE END ***************/

