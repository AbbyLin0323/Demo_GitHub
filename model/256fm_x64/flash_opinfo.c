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
*******************************************************************************/

//FLASH_ERROR_CONDITION
#include <time.h>
#include "BaseDef.h"
#include "Disk_Config.h"
#include "flash_mid.h"
#include "model_config.h"
#include "flash_opinfo.h"
#include "flash_meminterface.h"
#include "sim_flash_shedule.h"
#include "sim_flash_status.h"
#include "Sim_DEC_Status.h"
//#include "L2_ReadDisturb.h"


#define FLASH_MAX_ERASE_CNT     5000

FLASH_OP_STATISTICS l_FlashOpStatistics[NFC_MODEL_LUN_SUM] = { 0 };
FLASH_ERROR_CONDITION l_FlashErrCondition[20] = { 0 };
FLASH_CE_PRE_EraseErrSetting l_FlashPreBBSet[NFC_MODEL_LUN_SUM] = { 0 };

extern FLASH_RUNTIME_STATUS g_aFlashStatus;
extern U32 g_InjErrHead;

UINT64 l_FlashWriteCnt = 0;
UINT64 l_FlashReadCnt = 0;
BOOL l_bOutPutEraseCnt = FALSE;

extern char* SIM_GetLogFileFloder();
extern void Dump_InjectErrorInfo(FLASH_PHY* pflash_phy1, U8 PageType, U8 ErrType, U8 RetryTime);

void Flash_SetOutPutEraseCntFeature(BOOL bOpen)
{
    l_bOutPutEraseCnt = bOpen;
}

void Flash_EraseErrSetting(U8 ucCEIndex, U16 usBadBlockCnt)
{
    U16 usEraseIndex = 0;

    srand((U32)time(NULL));

    for (usEraseIndex = 0; usEraseIndex < usBadBlockCnt; usEraseIndex++)
    {
        U16 usEraseHit = 0;
        U16 usIndex = 0;
        do
        {
            usEraseHit = (rand() * rand()) % (BLK_PER_PLN + RSV_BLK_PER_PLN);
            for (usIndex = 0; usIndex < usEraseIndex; usIndex++)
            {
                if (usEraseHit == l_FlashPreBBSet[ucCEIndex].usEraseCntIndex[usIndex])
                {
                    usEraseHit = 0;
                    break;
                }
            }

        } while (usEraseHit == 0);

        l_FlashPreBBSet[ucCEIndex].usEraseCntIndex[usEraseIndex] = usEraseHit;
    }
    l_FlashPreBBSet[ucCEIndex].ucBadBlockCnt = (U8)usBadBlockCnt;
    return;
}

void Flash_PreErrSetting(U8 ucErrType)
{
    U32 uErrHit = 0;
    srand((U32)time(NULL));
    uErrHit = (rand() * rand()) % l_FlashErrCondition[ucErrType].ulHCondition;
    l_FlashErrCondition[ucErrType].ulLCondition = uErrHit;

    return;
}

void Flash_SetBadBlock(U8 ucCEIndex, U8 ucPlnIndex, U16 usBlockIndex)
{
    if (BLOCK_BAD != FlashStsM_GetBlkSts(ucCEIndex, ucPlnIndex, usBlockIndex))
    {
        FlashStsM_SetBlkSts(ucCEIndex, ucPlnIndex, usBlockIndex, BLOCK_BAD);
        FlashstsM_AddLunBBCnt(ucCEIndex);
    }
}

void Flash_OutCEBadBlock(void)
{
    FILE* pBadBlkFile = NULL;
    char strFileName[256] = { 0 };
    char *pstrLogFolder = SIM_GetLogFileFloder();
    U8 uCEIndex = 0;
    U16 usBlockIndex = 0;
    static U32 bInit = 0;

    //GetLocalTime(&sys_time);
    sprintf(strFileName, "%s\\BadBlk.xls", pstrLogFolder);


    if (bInit == 0)
    {
        bInit = 1;
        pBadBlkFile = fopen(strFileName, "w");
    }
    else
    {
        pBadBlkFile = fopen(strFileName, "a+");
    }

    if (NULL == pBadBlkFile)
    {
        DBG_Printf("Open %s fail\n", strFileName);
        DBG_Getch();
    }

    for (uCEIndex = 0; uCEIndex < NFC_MODEL_LUN_SUM; uCEIndex++)
    {

        fprintf(pBadBlkFile, "%d\t", FlashstsM_GetLunBBCnt(uCEIndex));
    }

    fprintf(pBadBlkFile, "\r");

    fclose(pBadBlkFile);

}

void FLASH_PreBBInit(U8 ucMaxBB, U8 ucMinBB)
{
    U8 ucRandomBB = 0;
    U8 ucCEIndex = 0;
    U16 uBBIndex = 0;
    U8 ucPlnIndex = 0;

    srand((U32)time(NULL));

    for (ucCEIndex = 0; ucCEIndex < NFC_MODEL_LUN_SUM; ucCEIndex++)
    {
        ucRandomBB = rand() % (ucMaxBB - ucMinBB);
        ucRandomBB += ucMinBB;
        for (uBBIndex = 0; uBBIndex < ucRandomBB; uBBIndex++)
        {
            U16 uBB = 0;
            do
            {
                //uBB = MLC_BLOCK_START + (rand() % (BLK_PER_PLN - MLC_BLOCK_START - 1));
                // for pass test error injection case, error block need be in last 200 blocks.
                uBB = BLK_PER_PLN - 200 + (rand() % 200);

                if (BLOCK_BAD == FlashStsM_GetBlkSts(ucCEIndex, ucPlnIndex, uBB))
                {
                    uBB = 0;
                }

            } while (uBB == 0);

            for(ucPlnIndex = 0; ucPlnIndex < PLN_PER_LUN; ucPlnIndex++)
            {
                Flash_SetBadBlock(ucCEIndex, ucPlnIndex, uBB);
                g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][uBB].bsInBBT = TRUE;
            }
        }
    }

    return;
}

void Flash_BBT_Init()
{
    U8 ucMaxBB = 0;
    U8 ucCEIndex = 0;
    U8 ucPlnIndex = 0;

#ifdef FLASH_L85
    for (ucCEIndex = 0; ucCEIndex < NFC_MODEL_LUN_SUM; ucCEIndex++)
    {
        for(ucPlnIndex = 0; ucPlnIndex < PLN_PER_LUN; ucPlnIndex++)
        {
            Flash_SetBadBlock(ucCEIndex, ucPlnIndex, 45);
            Flash_SetBadBlock(ucCEIndex, ucPlnIndex, 263);
            g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][45].bsInBBT = TRUE;
            g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][263].bsInBBT = TRUE;
        }
    }
    ucMaxBB = 6;
    FLASH_PreBBInit(ucMaxBB - 2, 0);
    Flash_OutCEBadBlock();
#else
    for (ucCEIndex = 0; ucCEIndex < NFC_MODEL_LUN_SUM; ucCEIndex++)
    {
        for (ucPlnIndex = 0; ucPlnIndex < 1; ucPlnIndex++)
        {
            /*Add All LUN's Blk1 to BBT */
            Flash_SetBadBlock(ucCEIndex, ucPlnIndex, 1);
            g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][1].bsInBBT = TRUE;

            /*Add All LUN's Blk2 to BBT */
            //Flash_SetBadBlock(ucCEIndex, ucPlnIndex, 2);
            //g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][2].bsInBBT = TRUE;
        }
    }
    ucMaxBB = 6;
    FLASH_PreBBInit(ucMaxBB, 0);
    Flash_OutCEBadBlock();
#endif

    return;
}

void Flash_OutPutEraseCnt(void)
{
    FILE* pEraseCntFile = NULL;

    SYSTEMTIME sys_time;
    char strFileName[256] = { 0 };
    char *pstrLogFolder = SIM_GetLogFileFloder();
    U8 uCEIndex = 0;
    U16 usBlockIndex = 0;
    U8 ucPlnIndex = 0;

    if (FALSE == l_bOutPutEraseCnt)
    {
        return;
    }

    GetLocalTime(&sys_time);
    sprintf(strFileName, "%s\\Flash_EraseCnt_%d_%d_%d.xls", pstrLogFolder, sys_time.wHour, sys_time.wMinute, sys_time.wSecond);


    pEraseCntFile = fopen(strFileName, "w");
    if (NULL == pEraseCntFile)
    {
        DBG_Printf("Open %s fail\n", strFileName);
        DBG_Getch();
    }

    for (uCEIndex = 0; uCEIndex < NFC_MODEL_LUN_SUM; uCEIndex++)
    {
        fprintf(pEraseCntFile, "CE:%d\t", uCEIndex);
    }

    fprintf(pEraseCntFile, "\r");


    for (usBlockIndex = 0; usBlockIndex < BLK_PER_PLN + RSV_BLK_PER_PLN; usBlockIndex++)
    {
        for(ucPlnIndex = 0; ucPlnIndex < PLN_PER_LUN; ucPlnIndex ++)
        {
            for (uCEIndex = 0; uCEIndex < NFC_MODEL_LUN_SUM; uCEIndex++)
            {
                fprintf(pEraseCntFile, "%d\t", FlashStsM_GetBlkPeCnt(uCEIndex, ucPlnIndex, usBlockIndex));
            }
        }
        fprintf(pEraseCntFile, "\r");
    }


    fclose(pEraseCntFile);

    return;
}

void Flash_SetErrCondition(U8 ucErrType, U32 ulHCondition, U32 ulLCondition)
{
    if (ucErrType >= 20)
    {
        DBG_Printf("Flash_SetErrCondition: Max ErrType = 19, you set err type = %d\r\n", ucErrType);
        DBG_Getch();
    }

    if ((NF_ERR_TYPE_ERS == ucErrType) &&
        (ulHCondition != 0 || ulLCondition != 0))
    {
        if ((ulHCondition == 0) && (ulLCondition == 0))
        {
            Flash_SetOutPutEraseCntFeature(FALSE);
        }
        else
        {
            Flash_SetOutPutEraseCntFeature(TRUE);
        }

    }

    l_FlashErrCondition[ucErrType].ulHCondition = ulHCondition;
    l_FlashErrCondition[ucErrType].ulLCondition = ulLCondition;

    return;
}

U8 Flash_GetErrorCode(U8 ucCEIndex, U8 ucPlnIndex, U16 usBlockIndex, U16 uPageIndex)
{
    U8 ucErrCode = NF_SUCCESS;
    U8 ucErrIndex = 0;
    ERR_INFO *pErrInfo = NULL;
    ERR_HANDLE *pBlockErrHandle = &g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][usBlockIndex].tBlockErrHandel;

    if (1 == pBlockErrHandle->bsErr)
    {
        for (ucErrIndex = 0; ucErrIndex < MAX_BLOCK_ERR; ucErrIndex++)
        {
            if (uPageIndex == pBlockErrHandle->ErrInfo[ucErrIndex].bsErrPage)
            {
                ucErrCode = pBlockErrHandle->ErrInfo[ucErrIndex].bsErrCode;
                break;
            }
        }
    }

    return ucErrCode;
}

ERR_INFO* Flash_GetErrorInfo(U8 ucLunInTotal, U8 ucPlnIndex, U16 usBlockIndex, U16 uPageIndex)
{
    U8 ucErrCode = NF_SUCCESS;
    U8 ucErrIndex = 0;
    ERR_INFO *pErrInfo = NULL;
    ERR_HANDLE *pBlockErrHandle = &g_aFlashStatus.aFlashBlkInfo[ucLunInTotal][ucPlnIndex][usBlockIndex].tBlockErrHandel;

    if (1 == pBlockErrHandle->bsErr)
    {
        for (ucErrIndex = 0; ucErrIndex < MAX_BLOCK_ERR; ucErrIndex++)
        {
            if (uPageIndex == pBlockErrHandle->ErrInfo[ucErrIndex].bsErrPage &&
                NF_SUCCESS != pBlockErrHandle->ErrInfo[ucErrIndex].bsErrCode)
            {
                //usErrCode = pBlockErrHandle->ErrInfo[ucErrIndex].bsErrCode;
                pErrInfo = &pBlockErrHandle->ErrInfo[ucErrIndex];
                break;
            }
        }
    }

    return pErrInfo;
}

void Flash_SetMaxRetryTime(U8 ucLunInTotal, U8 ucPlnIndex, U16 usBlockIndex, U16 uPageIndex)
{
    ERR_INFO *pErrInfo = Flash_GetErrorInfo(ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex);

    if (NULL != pErrInfo)
    {
        pErrInfo->bsRetryTime = MAX_READRETRY_CNT + 1;
    }

    return;
}

void Flash_ResetErrInfo(ERR_INFO *pErrInfo)
{
    if (NULL != pErrInfo)
    {
        pErrInfo->bsErrCode = NF_SUCCESS;
        pErrInfo->bsErrPage = 0;
        pErrInfo->bsRetryTime = 0;
        pErrInfo->bsOpStatus = 0;
    }


    return;
}

ERR_INFO* Flash_MallocNewErrInfo(U8 ucLunInTotal, U8 ucPlnIndex, U16 usBlockIndex)
{
    U8 ucErrIndex = 0;
    ERR_HANDLE *pBlockErrHandle = &g_aFlashStatus.aFlashBlkInfo[ucLunInTotal][ucPlnIndex][usBlockIndex].tBlockErrHandel;
    ERR_INFO *pErrInfo = NULL;

    for (ucErrIndex = 0; ucErrIndex < MAX_BLOCK_ERR; ucErrIndex++)
    {
        if (NF_SUCCESS == pBlockErrHandle->ErrInfo[ucErrIndex].bsErrCode)
        {
            //usErrCode = pBlockErrHandle->ErrInfo[ucErrIndex].bsErrCode;
            pErrInfo = &pBlockErrHandle->ErrInfo[ucErrIndex];
            break;
        }
    }

    return pErrInfo;
}

BOOL Flash_AddNewErr(U8 ucLunInTotal, U8 ucPlnIndex, U16 usBlockIndex, U16 uPageIndex, U8 ucErrCode, U8 ucRetryTime)
{
    ERR_INFO *pNewErrInfo = NULL;
    U8 ucReadRetry = 0;
    BOOL bRtn = FALSE;
    FLASH_PHY FlashAdress = { 0 };
    U8 ucIndex;

    FlashAdress.ucLunInTotal = ucLunInTotal;
    FlashAdress.nPln = ucPlnIndex;
    FlashAdress.nBlock = usBlockIndex;
    FlashAdress.nPage = uPageIndex;

    pNewErrInfo = Flash_GetErrorInfo(ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex);
    if (NULL != pNewErrInfo)
    {
        if ((pNewErrInfo->bsErrCode == ucErrCode) &&
            (pNewErrInfo->bsRetryTime == ucRetryTime))
        {
            return TRUE;
        }
        else
        {

            for (ucIndex = 0; ucIndex < g_InjErrHead; ucIndex++)
            {
                if (g_stFlashErrInjEntry[ucIndex].ucPhyPuInTotal == ucLunInTotal &&
                    g_stFlashErrInjEntry[ucIndex].block == usBlockIndex &&
                    g_stFlashErrInjEntry[ucIndex].page == uPageIndex &&
                    g_stFlashErrInjEntry[ucIndex].err_type == ucErrCode &&
                    g_stFlashErrInjEntry[ucIndex].retry_times == pNewErrInfo->bsRetryTime)
                {
                    g_stFlashErrInjEntry[ucIndex].valid = FALSE;
                    break;
                }
            }
            Flash_ResetErrInfo(pNewErrInfo);
        }
    }

    pNewErrInfo = Flash_MallocNewErrInfo(ucLunInTotal, ucPlnIndex, usBlockIndex);
    if (NULL != pNewErrInfo)
    {
        g_aFlashStatus.aFlashBlkInfo[ucLunInTotal][ucPlnIndex][usBlockIndex].tBlockErrHandel.bsErr = 1;
        pNewErrInfo->bsErrCode = ucErrCode;
        pNewErrInfo->bsErrPage = uPageIndex;
        pNewErrInfo->bsOpStatus = 0;

        // set read retry time for NF_ERR_TYPE_UECC
        if (NF_ERR_TYPE_UECC == ucErrCode)
        {
            if (0 != ucRetryTime)
            {
                if (ucRetryTime > (MAX_READRETRY_CNT + 1))
                {
                    pNewErrInfo->bsRetryTime = MAX_READRETRY_CNT + 1;
                }
                else
                {
                    pNewErrInfo->bsRetryTime = ucRetryTime;
                }
            }
            else
            {
                srand((U32)time(NULL));
                ucReadRetry = rand();
                if (0 == (ucReadRetry & 0x1))
                {
                    ucReadRetry = MAX_READRETRY_CNT + 1;
                }
                else
                {
                    ucReadRetry = (ucReadRetry % MAX_READRETRY_CNT) + 1;
                }
                pNewErrInfo->bsRetryTime = ucReadRetry;
            }
        }
        else if (NF_ERR_TYPE_ERS == ucErrCode)
        {
            pNewErrInfo->bsErrPage = INVALID_4F;
        }


        bRtn = TRUE;
    }

    Dump_InjectErrorInfo(&FlashAdress, 0, ucErrCode, ucRetryTime);

    return bRtn;
}

void Flash_ResetErrInfoInBlock(U8 ucCEIndex, U8 ucPlnIndex, U16 usBlockIndex)
{
    U8 ucErrIndex = 0;
    ERR_HANDLE *pBlockErrHandle = &g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][usBlockIndex].tBlockErrHandel;
    ERR_INFO *pErrInfo = NULL;

    for (ucErrIndex = 0; ucErrIndex < MAX_BLOCK_ERR; ucErrIndex++)
    {
        if (NF_SUCCESS != pBlockErrHandle->ErrInfo[ucErrIndex].bsErrCode)
        {
            pErrInfo = &pBlockErrHandle->ErrInfo[ucErrIndex];
            Flash_ResetErrInfo(pErrInfo);
        }
    }
}

void FlashStsM_AddTotalEraseCnt(U8 ucCEIndex, U32 ulEraseCnt)
{
    l_FlashOpStatistics[ucCEIndex].ulTotalEraseCnt += ulEraseCnt;
}

BOOL Flash_IsTriggerdErr(U8 ucCEIndex, U8 ucErrCode, BOOL bHCondition)
{
    BOOL bRtn = FALSE;
    U32 ulStatistics = 0;
    U32 ulCondition = 0;

    switch (ucErrCode)
    {
    case NF_ERR_TYPE_UECC:
        if (TRUE == bHCondition)
        {
            ulStatistics = l_FlashOpStatistics[ucCEIndex].ulHReadPageCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_UECC].ulHCondition;
        }
        else
        {
            ulStatistics = l_FlashOpStatistics[ucCEIndex].ulLReadPageCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_UECC].ulLCondition;
        }
        break;

    case NF_ERR_TYPE_RECC:
        if (TRUE == bHCondition)
        {
            ulStatistics = l_FlashOpStatistics[ucCEIndex].ulHReadPageCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_RECC].ulHCondition;
        }
        else
        {
            ulStatistics = l_FlashOpStatistics[ucCEIndex].ulLReadPageCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_RECC].ulLCondition;
        }
        break;
    case NF_ERR_TYPE_PRG:
        if (TRUE == bHCondition)
        {
            ulStatistics = l_FlashOpStatistics[ucCEIndex].ulHWritePageCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_PRG].ulHCondition;
        }
        else
        {
            ulStatistics = l_FlashOpStatistics[ucCEIndex].ulLWritePageCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_PRG].ulLCondition;
        }
        break;

    case NF_ERR_TYPE_ERS:
        /*if (TRUE == bHCondition)
        {
            ulStatistics = l_FlashOpStatistics.ulHEraseCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_ERS].ulHCondition;
        }
        else
        {
            ulStatistics = l_FlashOpStatistics.ulLEraseCnt;
            ulCondition = l_FlashErrCondition[NF_ERR_TYPE_ERS].ulLCondition;
        }*/
        break;

    }

    if ((ulCondition != 0) && (0 == ulStatistics % ulCondition))
    {
        bRtn = TRUE;
    }

    return bRtn;
}

void FlashStsM_AddPgRetryCnt(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage)
{
    l_FlashOpStatistics[ucLun].aPgRetryCnt[ucPln][usBlk][usPage]++;
    return;
}

U32 FlashStsM_GetPgRetryCnt(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage)
{
    return l_FlashOpStatistics[ucLun].aPgRetryCnt[ucPln][usBlk][usPage];
}

void FlashStsM_AddOpenBlkRdCnt(U8 ucLun, U8 ucPln, U16 usBlk)
{
    if(BLOCK_OPEN == FlashStsM_GetBlkSts(ucLun, ucPln, usBlk))
    {
        l_FlashOpStatistics[ucLun].ulOpenBlkRdCnt++;
    }

    return;
}

U32 FlashStsM_GetOpenBlkRdCnt(U8 ucLun)
{
    return l_FlashOpStatistics[ucLun].ulOpenBlkRdCnt;
}

void FlashStsM_AddOpenWLRdCnt(PFLASH_PHY pFlash_phy)
{
    if(PAIR_PAGE_OPEN == FlashStsM_GetPairPgSts(pFlash_phy))
    {
        l_FlashOpStatistics[pFlash_phy->ucLunInTotal].ulOpenWLRdCnt++;
    }

    return;
}

U32 FlashStsM_GetOpenWLRdCnt(U8 ucLun)
{
    return l_FlashOpStatistics[ucLun].ulOpenWLRdCnt;
}

void FlashStsM_AddNextOpenWLRdCnt(PFLASH_PHY pFlash_phy)
{
    U16 usCurPairPg;

    usCurPairPg = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock));

    if((PAIR_PAGE_PER_BLK != usCurPairPg) && (PAIR_PAGE_OPEN == FlashStsM_GetNextPairPgSts(pFlash_phy)))
    {
        l_FlashOpStatistics[pFlash_phy->ucLunInTotal].ulNextOpenWLRdCnt++;
    }

    return;
}

U32 FlashStsM_GetNextOpenWLRdCnt(U8 ucLun)
{
    return l_FlashOpStatistics[ucLun].ulNextOpenWLRdCnt;
}

void FlashStsM_AddCacheRdCnt(U8 ucLun)
{
    l_FlashOpStatistics[ucLun].ulCacheRdCnt++;
    return;
}

U32 FlashStsM_GetCacheRdCnt(U8 ucLun)
{
    return l_FlashOpStatistics[ucLun].ulCacheRdCnt;
}

void FlashStsM_AddCacheWrCnt(U8 ucLun)
{
    l_FlashOpStatistics[ucLun].ulCacheWrCnt++;
    return;
}

U32 FlashStsM_GetCacheWrCnt(U8 ucLun)
{
    return l_FlashOpStatistics[ucLun].ulCacheWrCnt;
}

BOOL Flash_BlockIsWornOut(U8 ucCEIndex, U8 ucPlnIndex, U16 usBlockIndex)
{
    U32 ulBlockType = 0;
    U32 ulMaxEraseCntLimit = FLASH_MAX_ERASE_CNT;

    return FALSE;

#if 0
    if (usBlockIndex < MLC_BLOCK_START)
    {
        ulMaxEraseCntLimit = FLASH_MAX_ERASE_CNT * 2;
    }

    if (FlashStsM_GetBlkPeCnt(ucCEIndex, ucPlnIndex, usBlockIndex) > FLASH_MAX_ERASE_CNT)
    {
        //NF_ERR_TYPE_ERS
        Flash_SetBadBlock(ucCEIndex, ucPlnIndex, usBlockIndex);
        Flash_AddNewErr(ucCEIndex, ucPlnIndex, usBlockIndex, 0xFFFF, NF_ERR_TYPE_ERS, 0);
    }

    return g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][usBlockIndex].bsBlockSts;
#endif
}

void Flash_SetMaxBlockArrayRandom(U8 ucCEIndex, U16 *pBlockIndex, U16 uBlockCnt)
{
    U16 *pIndexPointer = pBlockIndex;
    U16 uLoopIndex = 0;
    U16 uSearchIndex = 0;

    srand(ucCEIndex*(U32)time(NULL));

    for (uLoopIndex = 0; uLoopIndex < uBlockCnt; uLoopIndex++)
    {
        do
        {
            *pIndexPointer = rand() % BLK_PER_PLN;
            for (uSearchIndex = 0; uSearchIndex < uLoopIndex; uSearchIndex++)
            {
                // if random data equal, then repeat to random.
                if (*(pBlockIndex + uSearchIndex) == *pIndexPointer)
                {
                    *pIndexPointer = 0;
                    break;
                }
            }
        } while (*pIndexPointer < MLC_BLOCK_START);

        pIndexPointer++;
    }

}


/****************************************************************************
Name        :Flash_SetEraseCntForBlock
Input       :Flash phsical address
Output      :
Author      :bettywu
Date        :2015.4.22
Description :
Others      :
Modify      :
****************************************************************************/
// it is called when Set Flash status by test script
void Flash_SetEraseCntForBlock(U8 ucCEIndex, U8 ucPlnIndex, U16 usBlockIndex, U16 uEraseCnt)
{
    U32 ulBlockType = 0;
    U32 ulMaxEraseCntLimit = FLASH_MAX_ERASE_CNT;

    g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][usBlockIndex].bsPeCnt = uEraseCnt;
    FlashStsM_AddTotalEraseCnt(ucCEIndex, uEraseCnt);

    Flash_BlockIsWornOut(ucCEIndex, ucPlnIndex, usBlockIndex);
}

/****************************************************************************
Name        :Flash_OpErase
Input       :Flash phsical address
Output      :
Author      :bettywu
Date        :2015.4.22
Description :when erase, check error condition and if metch,
             produce NF_ERR_TYPE_ERS
Others      :
Modify      :
****************************************************************************/
// it is call when the block be erased
// increate erase count and judge if erase count exhausted
// return false have error
U8 Flash_OpErase(U8 ucCEIndex, U8 ucPlnIndex, U16 usBlockIndex, BOOL bsLLF)
{
    U8 ucErrCode = NF_SUCCESS;
    ERR_INFO *pErrInfo = Flash_GetErrorInfo(ucCEIndex, ucPlnIndex, usBlockIndex, 0xFFFF);

    /* check input param is correct */
    if (ucCEIndex >= NFC_MODEL_LUN_SUM || ucPlnIndex >= PLN_PER_LUN ||usBlockIndex >= (BLK_PER_PLN + RSV_BLK_PER_PLN))
    {
        DBG_Printf("Flash_OpErase : CEIndex = %d, PlnIndex = %d, BlockIndex = %d Error\n", ucCEIndex, ucPlnIndex, usBlockIndex);
        DBG_Getch();
    }

    /* erase block which set bad block on Flash_BBT_Init, return erase fail status */
    if (TRUE == g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][usBlockIndex].bsInBBT)
    {
        //DBG_Printf("Flash_OpErase: Erase block which has in model's BBT\n");
        //DBG_Printf("CEIndex = %d, PlnIndex = %d, BlockIndex = %d\n", ucCEIndex, ucPlnIndex, usBlockIndex);
        return NF_ERR_TYPE_ERS;
    }

    /* Erase program fail block, random return fail or success status */
    if (TRUE == g_aFlashStatus.aFlashBlkInfo[ucCEIndex][ucPlnIndex][usBlockIndex].bsIsPrgFail)
    {
        srand((U32)time(NULL));
        ucErrCode = (0 == rand() % 2) ? NF_SUCCESS : NF_ERR_TYPE_ERS;

        if (NF_SUCCESS == ucErrCode)
        {
            DBG_Printf("Erase prg fail block, assume erase success. Pu:%d, Pln:%d, Blk:%d\n", ucCEIndex, ucPlnIndex, usBlockIndex);
        }
        else
        {
            DBG_Printf("Erase prg fail block, assume erase fail. Pu:%d, Pln:%d, Blk:%d\n", ucCEIndex, ucPlnIndex, usBlockIndex);
        }
    }

    /* Add total erase cnt of the Lun */
    FlashStsM_AddTotalEraseCnt(ucCEIndex, 1);

    // Add block total erase cnt
    FlashStsM_AddBlkPeCnt(ucCEIndex, ucPlnIndex, usBlockIndex);

    // data block erase cnt in CE
    if (usBlockIndex >= MLC_BLOCK_START)
    {
        l_FlashOpStatistics[ucCEIndex].ulDataBlockEraseCnt++;

        // CE all block erase once, set bad block condition
        if (0 == (l_FlashOpStatistics[ucCEIndex].ulDataBlockEraseCnt % BLK_PER_PLN))
        {
            Flash_EraseErrSetting(ucCEIndex, l_FlashErrCondition[NF_ERR_TYPE_ERS].ulHCondition);
        }
    }

    if (NULL != pErrInfo)
    {
        ucErrCode = NF_ERR_TYPE_ERS;
    }
    else
    {
        Flash_ResetErrInfoInBlock(ucCEIndex, ucPlnIndex, usBlockIndex);

        if (TRUE == Flash_BlockIsWornOut(ucCEIndex, ucPlnIndex, usBlockIndex))
        {
            ucErrCode = NF_ERR_TYPE_ERS;
        }
        else
        {
            // if triger erase error
            if (usBlockIndex > MLC_BLOCK_START)
            {
                U16 uEraseIndex = 0;
                U16 uBBCnt = l_FlashPreBBSet[ucCEIndex].ucBadBlockCnt;
                for (uEraseIndex = 0; uEraseIndex < uBBCnt; uEraseIndex++)
                {
                    if (l_FlashOpStatistics[ucCEIndex].ulDataBlockEraseCnt % (BLK_PER_PLN + RSV_BLK_PER_PLN) == l_FlashPreBBSet[ucCEIndex].usEraseCntIndex[uEraseIndex])
                    {
                        ucErrCode = NF_ERR_TYPE_ERS;
                        break;
                    }
                }
            }
        }

    }

    if (NF_SUCCESS != ucErrCode)
    {
        //DBG_Printf("Flash_OpErase : CEIndex = %d, PlnIndex = %d, BlockIndex = %d Erase fail\n", ucCEIndex, ucPlnIndex, usBlockIndex);
        Flash_SetBadBlock(ucCEIndex, ucPlnIndex, usBlockIndex);
        Flash_AddNewErr(ucCEIndex, ucPlnIndex, usBlockIndex, 0xFFFF, ucErrCode, 0);
        FlashStsM_AddBlkEraseFailCnt(ucCEIndex, ucPlnIndex, usBlockIndex);
    }
    else if (NF_SUCCESS == ucErrCode)
    {
        FlashStsM_ResetSts(ucCEIndex, ucPlnIndex, usBlockIndex);
    }

    /* reset block program order */
    if (NULL != g_tFlashSpecInterface.ResetPrgOrder)
    {
        g_tFlashSpecInterface.ResetPrgOrder(ucCEIndex, usBlockIndex, ucPlnIndex);
    }

    return ucErrCode;
}

void Flash_OpBlkReadCntCheck(U8 usCEIndex, U8 ucPlnIndex, U16 uBlockIndex)
{
#if 0
    FLASH_BLOCK_STATUS *pFlashBlockStatus = &(g_aFlashStatus[usCEIndex][ucPlnIndex][uBlockIndex].aFlashBlkInfo);
    U16 usEraseCnt = pFlashBlockStatus->bsEraseCnt;
    BOOL bReadCntOverflow = FALSE;

    if (DATA_BLOCK_START_INDEX > uBlockIndex)
    {
        /* table block, needn't to update ReadCnt*/
        return;
    }

    if (TRUE == pFlashBlockStatus->bsOpen)
    {
        pFlashBlockStatus->ulReadCnt += TARGET_BLK_READ_CNT_DEC;
    }
    else
    {
        pFlashBlockStatus->ulReadCnt += CLOSED_BLK_READ_CNT_DEC;
    }

    if (READ_DISTURB_ERASE_CNT_THS_1 > usEraseCnt)
    {
        if (pFlashBlockStatus->ulReadCnt > BLK_READ_CNT_INIT_VALUE_THS_1)
        {
            bReadCntOverflow = TRUE;
        }
    }
    else if ((READ_DISTURB_ERASE_CNT_THS_1 <= usEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_2 > usEraseCnt))
    {
        if (pFlashBlockStatus->ulReadCnt > BLK_READ_CNT_INIT_VALUE_THS_2)
        {
            bReadCntOverflow = TRUE;
        }
    }
    else
    {
        if (pFlashBlockStatus->ulReadCnt > BLK_READ_CNT_INIT_VALUE_THS_3)
        {
            bReadCntOverflow = TRUE;
        }
    }

    if (TRUE == bReadCntOverflow)
    {
        DBG_Printf("Flash_OpBlkReadCntCheck: CE %d, Block 0x%x, EraseCnt 0x%x, ReadCnt 0x%x, ERROR!\n",
            usCEIndex, uBlockIndex, usEraseCnt, pFlashBlockStatus->ulReadCnt);
        DBG_Getch();
    }
#endif

    return;
}

/****************************************************************************
Name        :Flash_OpRead
Input       :Flash phsical address
Output      :
Author      :bettywu
Date        :2015.4.22
Description :when read, check error condition and if metch,
             produce NF_ERR_TYPE_UECC or NF_ERR_TYPE_RECC
Others      :
Modify      :
****************************************************************************/
// Flash op read
// return error code if read trigger a error
U8 Flash_OpRead(U8 ucLunInTotal, U8 ucPlnIndex, U16 usBlockIndex, U16 uPageIndex)
{
    ERR_HANDLE *pBlockErrHandle = &g_aFlashStatus.aFlashBlkInfo[ucLunInTotal][ucPlnIndex][usBlockIndex].tBlockErrHandel;
    ERR_INFO *pErrInfo = NULL;
    U8 ucErrCode = 0;

    if (ucLunInTotal >= NFC_MODEL_LUN_SUM || ucPlnIndex >= PLN_PER_LUN ||usBlockIndex >= (BLK_PER_PLN + RSV_BLK_PER_PLN) || uPageIndex >= (PG_PER_BLK * INTRPG_PER_PGADDR))
    {
        DBG_Printf("Flash_OpRead : ucLunInTotal = %d, PlnIndex = %d, BlockIndex = %d , PageIndex = %d, Error", ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex);
        DBG_Getch();
    }

    l_FlashReadCnt++;

    if ((l_FlashErrCondition[NF_ERR_TYPE_UECC].ulHCondition != 0) &&
        (0 == l_FlashReadCnt % l_FlashErrCondition[NF_ERR_TYPE_UECC].ulHCondition))
    {
        Flash_PreErrSetting(NF_ERR_TYPE_UECC);
    }
    if ((l_FlashErrCondition[NF_ERR_TYPE_RECC].ulHCondition != 0) &&
        (0 == l_FlashReadCnt % l_FlashErrCondition[NF_ERR_TYPE_RECC].ulHCondition))
    {
        Flash_PreErrSetting(NF_ERR_TYPE_RECC);
    }

    // produce a error
    if (usBlockIndex > MLC_BLOCK_START)
    {
        if ((l_FlashErrCondition[NF_ERR_TYPE_UECC].ulHCondition != 0) &&
            ((l_FlashReadCnt % l_FlashErrCondition[NF_ERR_TYPE_UECC].ulHCondition) == l_FlashErrCondition[NF_ERR_TYPE_UECC].ulLCondition))
        {
            ucErrCode = NF_ERR_TYPE_UECC;
        }
        else if ((l_FlashErrCondition[NF_ERR_TYPE_RECC].ulHCondition != 0) &&
            ((l_FlashReadCnt % l_FlashErrCondition[NF_ERR_TYPE_RECC].ulHCondition) ==
            l_FlashErrCondition[NF_ERR_TYPE_RECC].ulLCondition))
        {
            ucErrCode = NF_ERR_TYPE_RECC;
        }

        // handle a new error
        if (NF_SUCCESS != ucErrCode)
        {
            if (FALSE == Flash_AddNewErr(ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex, ucErrCode, 0))
            {
                ucErrCode = NF_SUCCESS;
            }
        }
    }

    pErrInfo = Flash_GetErrorInfo(ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex);

    if (NULL != pErrInfo)
    {
        // if not read error then getch

        // if have a UECC, check retry time
        if (NF_ERR_TYPE_UECC == pErrInfo->bsErrCode)
        {
            if (pErrInfo->bsRetryTime == pErrInfo->bsOpStatus)
            {
                Flash_ResetErrInfo(pErrInfo);
                ucErrCode = NF_SUCCESS;
            }
            else
            {
                ucErrCode = NF_ERR_TYPE_UECC;
                pErrInfo->bsOpStatus++;
                if (pErrInfo->bsOpStatus == (MAX_READRETRY_CNT + 1))
                {
                    pErrInfo->bsOpStatus = 0;
                }
            }
        }
        else
        {
            ucErrCode = pErrInfo->bsErrCode;
        }
    }

    if (NF_SUCCESS == ucErrCode)
    {
        //Flash_OpBlkReadCntCheck(ucCEIndex, ucPlnIndex, usBlockIndex);
    }
    return ucErrCode;
}


/****************************************************************************
Name        :Flash_OpWrite
Input       :Flash phsical address
Output      :
Author      :bettywu
Date        :2015.4.22
Description :when write, check error condition and if metch,
             produce NF_ERR_TYPE_PRG
Others      :
Modify      :
****************************************************************************/
// Flash op Write
// return error code if write trigger a error
U8 Flash_OpWrite(U8 ucLunInTotal, U8 ucPlnIndex, U16 usBlockIndex, U16 uPageIndex)
{
    ERR_HANDLE *pBlockErrHandle = &g_aFlashStatus.aFlashBlkInfo[ucLunInTotal][ucPlnIndex][usBlockIndex].tBlockErrHandel;
    ERR_INFO *pErrInfo = Flash_GetErrorInfo(ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex);
    U8 ucErrCode = 0;

    if (ucLunInTotal >= NFC_MODEL_LUN_SUM ||ucPlnIndex >= PLN_PER_LUN ||usBlockIndex >= (BLK_PER_PLN + RSV_BLK_PER_PLN) || uPageIndex >= (PG_PER_BLK * INTRPG_PER_PGADDR))
    {
        DBG_Printf("Flash_OpWrite : LunInTotal = %d, PlnIndex = %d, BlockIndex = %d , PageIndex = %d, Error \n", ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex);
        DBG_Getch();
    }

     /* check block erased fail */
    if(TRUE == FlashStsM_CheckEraseFail(ucLunInTotal, ucPlnIndex, usBlockIndex))
    {
        DBG_Printf("Write erased fail block !! LunInTotal%d Blk%d Pln%d\n", ucLunInTotal, ucPlnIndex, usBlockIndex);
        DBG_Getch();
    }

    if (uPageIndex == 0)
    {
        if (BLOCK_ERASED != FlashStsM_GetBlkSts(ucLunInTotal, ucPlnIndex, usBlockIndex))
        {
            DBG_Printf("Flash_OpWrite:LunInTotal = %d, PlnIndex = %d, usBlockIndex = %d rewrite\n", ucLunInTotal, ucPlnIndex, usBlockIndex);
            DBG_Getch();
        }

        FlashStsM_SetBlkSts(ucLunInTotal, ucPlnIndex, usBlockIndex, BLOCK_OPEN);
    }
    else if (uPageIndex >= (LOGIC_PG_PER_BLK - 1))
    {
        FlashStsM_SetBlkSts(ucLunInTotal, ucPlnIndex, usBlockIndex, BLOCK_CLOSE);
    }

    l_FlashOpStatistics[ucLunInTotal].ulTotalWriteCnt++;
    l_FlashWriteCnt++;
    if ((0 != l_FlashErrCondition[NF_ERR_TYPE_PRG].ulHCondition) &&
        (0 == l_FlashWriteCnt % l_FlashErrCondition[NF_ERR_TYPE_PRG].ulHCondition))
    {
        Flash_PreErrSetting(NF_ERR_TYPE_PRG);
    }
    if (NULL != pErrInfo)
    {
        if (NF_ERR_TYPE_PRG != pErrInfo->bsErrCode)
        {
            DBG_Printf("Flash_OpWrite : LunInTotal = %d, PlnIndex = %d, BlockIndex = %d , PageIndex = %d, Error = %d \n", ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex, pErrInfo->bsErrCode);
            DBG_Getch();
        }
        else
        {
            ucErrCode = pErrInfo->bsErrCode;
        }
    }

    // add WritePageCnt
    if (NF_SUCCESS == ucErrCode && usBlockIndex > MLC_BLOCK_START)
    {
        if ((0 != l_FlashErrCondition[NF_ERR_TYPE_PRG].ulHCondition) &&
            ((l_FlashWriteCnt % l_FlashErrCondition[NF_ERR_TYPE_PRG].ulHCondition) ==
            l_FlashErrCondition[NF_ERR_TYPE_PRG].ulLCondition))
        {
            ucErrCode = NF_ERR_TYPE_PRG;
        }

        // add a new error
        if (NF_SUCCESS != ucErrCode)
        {
            if (FALSE == Flash_AddNewErr(ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex, ucErrCode, 0))
            {
                ucErrCode = NF_SUCCESS;
            }
        }

    }

    if(NF_SUCCESS != ucErrCode)
    {
        if (TRUE == g_aFlashStatus.aFlashBlkInfo[ucLunInTotal][ucPlnIndex][usBlockIndex].bsIsPrgFail)
        {
            /* if program fail a block second times, add block to bbt */
            DBG_Printf("Pu:%d, Pln:%d, Blk:%d program fail second times\n", ucLunInTotal, ucPlnIndex, usBlockIndex);
            Flash_SetBadBlock(ucLunInTotal, ucPlnIndex, usBlockIndex);
            Flash_AddNewErr(ucLunInTotal, ucPlnIndex, usBlockIndex, uPageIndex, ucErrCode, 0);
        }
        else
        {
            g_aFlashStatus.aFlashBlkInfo[ucLunInTotal][ucPlnIndex][usBlockIndex].bsIsPrgFail = TRUE;
        }
    }

    // when write per 0x100000 pages , ouput erase cnt.
    if ((l_FlashWriteCnt % 0x100000) == 0)
    {
        Flash_OutPutEraseCnt();
        Flash_OutCEBadBlock();
    }

    return ucErrCode;
}


U8 Flash_StsReport(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U8 ucNfcqDepth, PLANE_ERR_TYPE aPlaneErrType, U8 ucPln)
{
    U8 usErrCode = aPlaneErrType.ErrCode;

    //Report DEC status according to different command code
    if (CMD_CODE_ERASE == pFlashCmdParam->bsCmdType || CMD_CODE_PROGRAM == pFlashCmdParam->bsCmdType)
    {
        DecStsM_SetFlashSts(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct), 0, aPlaneErrType.ErrSts);

    }
    else if (CMD_CODE_READ == pFlashCmdParam->bsCmdType)
    {
        usErrCode = DecStsM_Set_SRAM_FIFO(tNfcOrgStruct, pFlashCmdParam, aPlaneErrType, ucPln);
    }

    return usErrCode;
}
