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

#include "Disk_Config.h"
#include "flash_mid.h"
#include "flash_meminterface.h"
#include "model_config.h"
#include "flash_opinfo.h"
#include "sim_flash_shedule.h"

#include "MICRON_MLC_L95/flash_special_interface.h"
#include "TSB_FOURPLN/flash_special_interface.h"
#include "TSB_3D_TLC/flash_special_interface.h"
#include "TSB_2D_TLC/flash_special_interface.h"
#include "INTEL_3D_TLC/flash_special_interface.h"
#include "IM_3D_TLC_GEN2/flash_special_interface.h"
#include "sim_flash_status.h"

#define SATA_MAX_SECTION            (((MAX_LPN_IN_SYSTEM << SEC_PER_LPN_BITS) / SATA_LBA_PER_SECTION) + (((MAX_LPN_IN_DISK_MAX << SEC_PER_LPN_BITS)%SATA_LBA_PER_SECTION)? 1:0))

#define SATA_LBA_IN_SECTION(ulLBA)  (ulLBA/SATA_LBA_PER_SECTION)

extern char* SIM_GetLogFileFloder();

FlashSpecInterface g_tFlashSpecInterface = {0};

SATA_RAM_DEF g_SATARamData[SATA_MAX_SECTION] = {0};
FLASH_COM_PU_DEF g_FlashCOMData[NFC_MODEL_LUN_SUM] = { 0 };
FLASH_TABLE_PU_DEF g_FlashTableData[NFC_MODEL_LUN_SUM] = { 0 };

void Flash_Data_Handle(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, UINT32 nDataLen, BOOL bWrite);
void Flash_Table_Handle(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, UINT32 nDataLen, BOOL bWrite);

FLASH_OP_LOG l_FlashOpLog[NFC_MODEL_LUN_SUM] = { 0 };

extern U8 g_LPNInRedOffSet;
extern FLASH_RUNTIME_STATUS g_aFlashStatus;

//local function

// function from flash_opinfo.c
extern void Flash_SetMaxBlockArrayRandom(U8 usCEIndex, U16 *pBlockIndex, U16 uBlockCnt);
extern void Flash_SetEraseCntForBlock(U8 usCEIndex, U8 ucPlnIndex, U16 uBlockIndex, U16 uEraseCnt);
extern BOOL Flash_UpdateEraseCnt(U8 nPU, U16 nBlock);
extern U8 Flash_GetErrorCode(U8 usCEIndex, U8 ucPlnIndex, U16 uBlockIndex, U16 uPageIndex);
extern void Flash_SetErrCondition(U8 usErrType, U32 ulHCondition, U32 ulLCondition);
extern U8 Flash_OpRead(U8 ucLunInTotal, U8 ucPlnIndex, U16 uBlockIndex, U16 uPageIndex);
extern U8 Flash_OpWrite(U8 ucLunInTotal, U8 ucPlnIndex, U16 uBlockIndex, U16 uPageIndex);
extern U8 Flash_OpErase(U8 usCEIndex, U8 ucPlnIndex, U16 uBlockIndex, BOOL bsLLF);
extern U8 Flash_StsReport(const NFCM_LUN_LOCATION * tNfcOrgStruct, ST_FLASH_CMD_PARAM * pFlashCmdParam, U8 ucNfcqDepth, PLANE_ERR_TYPE aPlaneErrType, U8 ucPln);
extern void Flash_OutPutEraseCnt(void);
extern void Flash_BBT_Init();
extern void Flash_SetMaxRetryTime(U8 ucLunInTotal, U8 ucPlnIndex, U16 uBlockIndex, U16 uPageIndex);
extern BOOL Flash_AddNewErr(U8 ucLunInTotal, U8 ucPlnIndex, U16 uBlockIndex, U16 uPageIndex, U8 usErrCode, U8 usRetryTime);
extern void Dbg_IncDevDataReadCnt(int PageCnt);
extern void Dbg_IncDevTableReadCnt(int PageCnt);
extern void Dbg_IncDevDataWriteCnt(int PageCnt);
extern void Dbg_IncDevTableWriteCnt(int PageCnt);
extern void FlashStsM_AddOpenBlkRdCnt(U8 ucLun, U8 ucPln, U16 usBlk);
extern void FlashStsM_AddOpenWLRdCnt(PFLASH_PHY pFlash_phy);
extern void FlashStsM_AddNextOpenWLRdCnt(PFLASH_PHY pFlash_phy);

void Mid_FlashOpLog(char op, PFLASH_PHY pFlash_phy, U32 *pLpnAddr)
{
    U16 uCE = pFlash_phy->ucLunInTotal;
    U32 ulLogIndex = l_FlashOpLog[uCE].FlashLogIndex + 1;

    if (ulLogIndex >= FLASH_OP_LOG_MAX)
    {
        ulLogIndex = 0;
    }

    memcpy((void*)&l_FlashOpLog[uCE].FlashOpLog[ulLogIndex].DataLPN, pLpnAddr, LPN_PER_BUF*4);
    memcpy((void*)&l_FlashOpLog[uCE].FlashOpLog[ulLogIndex].FlashAddr, pFlash_phy, sizeof(FLASH_PHY));
    l_FlashOpLog[uCE].FlashOpLog[ulLogIndex].FlashOP = op;
    l_FlashOpLog[uCE].FlashLogIndex = ulLogIndex;

}

void Mid_PrintfFlashLog()
{
    FILE* pLogFile = NULL;
    char* pStrLogFolder = SIM_GetLogFileFloder();
    char strLogFileName[256];
    U32 ulLogIndex = 0;
    U32 ulNextLogIndex = 0;
    U16 usCEIndex = 0;
    U32 ulLoopIndex = 0;

    for (usCEIndex = 0; usCEIndex < NFC_MODEL_LUN_SUM; usCEIndex++)
    {
        PFLASH_PHY pFlashPhy;
        sprintf(strLogFileName, "%s\\FlashOpLog_%d.txt", pStrLogFolder, usCEIndex);
        pLogFile = fopen(strLogFileName,"w");
        if (NULL != pLogFile)
        {
            ulNextLogIndex = l_FlashOpLog[usCEIndex].FlashLogIndex + 1;
            if (ulNextLogIndex >= FLASH_OP_LOG_MAX)
            {
                ulNextLogIndex = 0;
            }
            pFlashPhy = &l_FlashOpLog[usCEIndex].FlashOpLog[ulNextLogIndex].FlashAddr;

            //if ulNextLog is empty, ulLogIndex not full
            if (0 == pFlashPhy->u64Addr)
            {
                ulLogIndex = 0;
            }
            else
            {
                ulLogIndex = ulNextLogIndex;
            }

            // print data to file
            do //(; ulLogIndex != l_FlashOpLog[usCEIndex].FlashLogIndex; ulLogIndex = (++ulLogIndex) % FLASH_OP_LOG_MAX)
            {
                FLASH_OP_LOG_ENTRY *pFlashLog = &(l_FlashOpLog[usCEIndex].FlashOpLog[ulLogIndex]);
                U8 uLPNIndex = 0;
                fprintf(pLogFile,"ulLogIndex:%d\tPU = 0x%x\tBLOCK = 0x%x\tPAGE = 0x%x\tOP = %c\t",
                    ulLogIndex, pFlashLog->FlashAddr.ucLunInTotal, pFlashLog->FlashAddr.nBlock, pFlashLog->FlashAddr.nPage, pFlashLog->FlashOP);

                for(uLPNIndex = 0; uLPNIndex < 8; uLPNIndex++)
                {
                    fprintf(pLogFile, "LPN_%d:0x%x\t", uLPNIndex, pFlashLog->DataLPN[uLPNIndex]);
                }
                fprintf(pLogFile, "\n");

                ulLogIndex = (++ulLogIndex) % FLASH_OP_LOG_MAX;
            }while (ulLogIndex != ulNextLogIndex);

            fclose(pLogFile);
        }// open file success
    }// end for usCEIndex

}




void Mid_FlashSetBlockErase(U16 uMaxEraseCnt,U16 uMinEraseCnt,U32 ulMaxPercent)
{
    U8 usCEIndex = 0;
    U8 ucPlnIndex = 0;
    U16 usMaxBlockIndex[512] = { 0 };
    U16 uMaxEraseBlockLimit = ((BLK_PER_PLN + RSV_BLK_PER_PLN) * ulMaxPercent) / 100;
    U16 uEraseCntGap = uMaxEraseCnt - uMinEraseCnt;

    U16 uRandom = 0;
    U16 uBlockLoop = 0;
    U16 uIndex = 0;


    //FlashStsM_InitFlashStatus();
    if (0 == uMaxEraseCnt && 0 == uMinEraseCnt)
    {

        return;
    }

    for (usCEIndex = 0; usCEIndex < NFC_MODEL_LUN_SUM; usCEIndex++)
    {
        srand(usCEIndex);
        for(ucPlnIndex = 0; ucPlnIndex < PLN_PER_LUN; ucPlnIndex++)
        {
            srand(ucPlnIndex);
            //set Max erase cnt
            Flash_SetMaxBlockArrayRandom(usCEIndex, usMaxBlockIndex, uMaxEraseBlockLimit);
            for (uIndex = 0; uIndex < uMaxEraseBlockLimit; uIndex++)
            {
                Flash_SetEraseCntForBlock(usCEIndex, ucPlnIndex, usMaxBlockIndex[uIndex], uMaxEraseCnt);
            }

            //set random erase cnt
            for (uBlockLoop = 0; uBlockLoop < BLK_PER_PLN + RSV_BLK_PER_PLN; uBlockLoop++)
            {
                uRandom = (rand() * rand()) % uEraseCntGap;
                if (0 == FlashStsM_GetBlkPeCnt(usCEIndex, ucPlnIndex, uBlockLoop))
                {
                    Flash_SetEraseCntForBlock(usCEIndex, ucPlnIndex, uBlockLoop, uMinEraseCnt + uRandom);
                }
            }
        }
    }

    Flash_OutPutEraseCnt();
}

void Flash_CalulateErrCondition(U16 uErrRate, U32 *pHCondition, U32 *pLCondition)
{
    U32 ulCondition = 0;

    if (uErrRate != 0)
    {
        ulCondition = 10000 / uErrRate;
    }

    //ulCondition = 10000 / uUECCRate;
    *pHCondition = ulCondition - ulCondition / 2;
    *pLCondition = ulCondition + ulCondition / 2;

}

void Mid_FlashSetErrRate(U16 uUECCRate, U16 uRECCRate, U16 uPragramErrRate, U16 uEraseErrRate)
{
    U32 ulHCondition = 0;
    U32 ulLCondition = 0;
    U32 ulCondition = 0;
    U32 ulPagePerCE = LOGIC_PG_PER_BLK * (BLK_PER_PLN + RSV_BLK_PER_PLN);

    //Flash_CalulateErrCondition(uUECCRate, &ulHCondition, &ulLCondition);
    ulHCondition = uUECCRate;
    Flash_SetErrCondition(NF_ERR_TYPE_UECC, ulHCondition, ulLCondition);

    //Flash_CalulateErrCondition(uRECCRate, &ulHCondition, &ulLCondition);
    ulHCondition = uRECCRate;
    Flash_SetErrCondition(NF_ERR_TYPE_RECC, ulHCondition, ulLCondition);

    //Flash_CalulateErrCondition(uPragramErrRate, &ulHCondition, &ulLCondition);
    ulHCondition = uPragramErrRate;
    Flash_SetErrCondition(NF_ERR_TYPE_PRG, ulHCondition, ulLCondition);

    //Flash_CalulateErrCondition(uEraseErrRate, &ulHCondition, &ulLCondition);
    //ulHCondition = ulHCondition * 8 * 10;
    //ulLCondition = ulLCondition * 8 * 10;
    Flash_SetErrCondition(NF_ERR_TYPE_ERS, uEraseErrRate, uEraseErrRate);

}

BOOL ModuleMalloc(void **pBufAddr, unsigned long nBufLen)
{
    char *pBuffer = 0;
    BOOL bRtn = FALSE;
    DWORD dwErrorID = 0;

    pBuffer = (char *)VirtualAlloc(NULL, nBufLen, MEM_COMMIT|MEM_RESERVE|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);

    if (NULL == pBuffer)
    {
        dwErrorID = GetLastError();
        DBG_Printf("VirtualAlloc failed. ErrID = %d", dwErrorID);
        DBG_Getch();
    }

    if (NULL != pBuffer)
    {

        *pBufAddr = pBuffer;
        bRtn = TRUE;
    }



    return bRtn;
}

//SATA operation
int Mid_init_SATA()
{

    unsigned long nLbaPerSection = SATA_LBA_PER_SECTION;
    int nMaxSection = SATA_MAX_SECTION;
    int nLbaCurSection = 0;
    int nLoop = 0;
    BOOL bRtn = FALSE;
    UINT32 nMaxLBA = MAX_LBA_IN_SYSTEM;

    // malloc memory every 1G
    for (nLoop = 0; nLoop < nMaxSection; nLoop++)
    {
        g_SATARamData[nLoop].ulStartLBA = nLoop * nLbaPerSection;

        if ((nLoop + 1) * nLbaPerSection > nMaxLBA)
        {
            nLbaCurSection = nMaxLBA - nLoop * nLbaPerSection;
        }
        else
        {
            nLbaCurSection = nLbaPerSection;
        }
        g_SATARamData[nLoop].ulLen = nLbaCurSection;

        bRtn = ModuleMalloc((void **)&g_SATARamData[nLoop].pDataAddr,
                            nLbaCurSection*SATA_LBA_DATA_LEN);

        if (FALSE == bRtn)
        {
            DBG_Printf("Mid_init_SATA malloc memory error \n");
            DBG_Getch();
            break;
        }

    }

    return bRtn;
}

void Mid_Init_Ex()
{
    UINT32 nPu = NFC_MODEL_LUN_SUM;
    UINT32 nPuIndex = 0;
    UINT32 nDataBlockPerPln = DATA_BLOCK_PER_PLN;
    UINT32 nTableBlockPerPln = NORMAL_BLOCK_ADDR_BASE;
    UINT32 nPagePerBlock = LOGIC_PG_PER_BLK;
    UINT32 nPlnIndex = 0;
    UINT32 nPlnPerPu = PLN_PER_LUN;
    UINT8 **pAddr = NULL;
    BOOL bRtn = FALSE;

    for(nPuIndex = 0; nPuIndex < nPu; nPuIndex++)
    {
        g_FlashCOMData[nPuIndex].nBlockPerPln = nDataBlockPerPln;
        g_FlashCOMData[nPuIndex].nPagePerPln = nPagePerBlock;

        g_FlashTableData[nPuIndex].nBlockPerPln = nTableBlockPerPln;
        g_FlashTableData[nPuIndex].nPagePerPln = nPagePerBlock;
        l_FlashOpLog[nPuIndex].FlashLogIndex = FLASH_OP_LOG_MAX;

        for (nPlnIndex = 0; nPlnIndex < nPlnPerPu; nPlnIndex++)
        {
            // malloc for data pln
            pAddr = (UINT8**)&g_FlashCOMData[nPuIndex].pDataPln[nPlnIndex];
            bRtn = ModuleMalloc((void**)pAddr, sizeof(FLASH_DATA_PLN));
            if (bRtn)
                memset(*pAddr, 0xFF, sizeof(FLASH_DATA_PLN));

            // malloc for table pln
            if (bRtn)
            {
                pAddr = (UINT8**)&g_FlashTableData[nPuIndex].pTablePln[nPlnIndex];
                bRtn = ModuleMalloc((void**)pAddr, sizeof(FLASH_TABLE_PLN));
                if (bRtn)
                {
                    memset(*pAddr, 0xFF,sizeof(FLASH_TALBE_BLOCK_MANT)*TOTAL_BLOCK_PER_PLN);
                    memset((*pAddr + sizeof(FLASH_TALBE_BLOCK_MANT)*TOTAL_BLOCK_PER_PLN), 0xFF, sizeof(FLASH_TALBE_PAGE_MANT)*TABLE_PAGE_PER_PLN);
                }
                //if (bRtn)
                //    memset(*pAddr, 0xFF, sizeof(FLASH_TABLE_PLN));
            }

            if (!bRtn)
            {
                DBG_Printf("Mid_Init_Ex, malloc memory error\n");
                DBG_Getch();
                break;
            }
        }
    }

    FlashStsM_InitFlashStatus();
#ifndef RDT_SORT
    Flash_BBT_Init();
#endif
}

U32* GetSATADstAddr(UINT32 ulLBA)
{
    UINT32 nSectionID = SATA_LBA_IN_SECTION(ulLBA);
    UINT32 *pDataAddr = g_SATARamData[nSectionID].pDataAddr;
    UINT32 nPosInSection = ulLBA % SATA_LBA_PER_SECTION;

    return (pDataAddr + nPosInSection);
}

void Mid_Read_SATA(UINT32 ulLBA, char* pBuf, int nLength)
{
    UINT32 *pDataAddr = GetSATADstAddr(ulLBA);
    memcpy(pBuf, pDataAddr, nLength);
}

void Mid_Write_SATA(unsigned long ulLBA, char* pBuf, int nLength)
{
    UINT32 *pDataAddr = GetSATADstAddr(ulLBA);
    memcpy(pDataAddr, pBuf,nLength);
}

void Mid_Erase_SATA(unsigned long ulLBA)
{
    UINT32 *pDataAddr = GetSATADstAddr(ulLBA);
    memset(pDataAddr, 0xFF, 4);
}

BOOL Mid_FlashInjError(PFLASH_PHY pFlash_phy, U8 usErrCode, U8 usRetryTime)
{
    BOOL bRtn = FALSE;

    bRtn = Flash_AddNewErr(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage, usErrCode, usRetryTime);

    return bRtn;
}

U8 Mid_Write(PFLASH_PHY pFlash_phy, int nType, char* pDataBuf, char * pRedBuf, int nLength, BOOL bCheckErr, ST_FLASH_CMD_PARAM *pFlashCmdParam, const NFCM_LUN_LOCATION *tNfcOrgStruct, U32 ulSeedIndex)
{
    U8 usErrCode = NF_SUCCESS;
    U8 usErrSts = 0;/*support P_ERR_INJ_DEC_REP Test*/
    U8 ucCmdType = pFlashCmdParam->bsCmdType;
    BOOL bsTLCMode = pFlashCmdParam->bsIsTlcMode;
    BOOL bNeedWriteToFlash = TRUE;
    PLANE_ERR_TYPE l_aPlaneErrType;
    U8 ucPrgCycle = 0;
    static U16 usFstDsgPtr;

    /* Check cmd mode */
    if (FlashstsM_GetLunMode(pFlash_phy->ucLunInTotal) != NFC_GetCmdMode(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct)))
    {
        DBG_Printf("Write command work mode error\n");
        DBG_Printf("LunMode: %d, CmdMode: %d\n", FlashstsM_GetLunMode(pFlash_phy->ucLunInTotal), NFC_GetCmdMode(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct)));
        DBG_Getch();
    }

    if(BLOCK_INVALID == FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock) ||
        BLOCK_TLC_LOWPG == FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock))
    {
        if (g_tFlashSpecInterface.SetBlkType != NULL)
        {
            g_tFlashSpecInterface.SetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage, ucCmdType, bsTLCMode);
        }
        else
        {
            DBG_Printf("Mid_Write: SetBlkType interface NULL!! \n");
            DBG_Getch();
        }
    }

    /* check block type */
    if(g_tFlashSpecInterface.CheckBlkType != NULL &&
        FALSE == g_tFlashSpecInterface.CheckBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, ucCmdType, bsTLCMode))
    {
        DBG_Printf("The cmd can't operate the block type!! LunInTotal%d Blk%d Pln%d CmdType%d TLCmode%d\n", pFlash_phy->ucLunInTotal, pFlash_phy->nBlock, pFlash_phy->nPln, ucCmdType, bsTLCMode);
        DBG_Getch();
    }

    /* check pair page program fail */
    /* Using FstDsgPtr indicates whether others write cmd program on failed pair page */
    if (usFstDsgPtr != pFlashCmdParam->p_nf_cq_entry->bsFstDsgPtr && TRUE == FlashStsM_CheckPrgFail(pFlash_phy))
    {
        DBG_Printf("Lun %d Block %d Program failed pair page: Page%d !!\n", pFlash_phy->ucLunInTotal, pFlash_phy->nBlock, pFlash_phy->nPage);
        DBG_Getch();
    }

    /* check scramble seed if write */
    if(TRUE == FlashStsM_GetLpMpUpWrite(pFlash_phy))
    {
        if (FALSE == FlashStsM_CheckPairPgSrcSeed(FlashStsM_GetPairPgSrcSeed(pFlash_phy), pFlashCmdParam->p_nf_cq_entry, ulSeedIndex))
        {
            DBG_Printf("PU %d, Pln %d, Blk %d, Pg %d, Write scramble seed error!\n", pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
            DBG_Getch();
        }
    }

    /* if first time program pair page, set it's status to open*/
    if((FLASH_LOW_PAGE == g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock))) &&
        (0 == FlashStsM_GetPairPgPrgStep(pFlash_phy)))
    {
        /* check pair page status is free */
        if(PAIR_PAGE_FREE != FlashStsM_GetPairPgSts(pFlash_phy))
        {
            DBG_Printf("Pair page is not free !!  Status = %d \n", FlashStsM_GetPairPgSts(pFlash_phy));
            DBG_Getch();
        }
        FlashStsM_SetPairPgSts (pFlash_phy, PAIR_PAGE_OPEN);
    }

    /* check pair page program (not check data) */
    /* In normal pair page, it must follow the pair page program theorem*/
    /* If a failed pair page, it can be ignore it.*/
    if (FALSE == FlashStsM_CheckPrgFail(pFlash_phy) && FALSE == FlashStsM_CheckPairPgPrg(pFlash_phy))
    {
        DBG_Getch();
    }

    /* check program order : on need to check the slc mode program order */
    if (NULL != g_tFlashSpecInterface.CheckPrgOrder)
    {
        bNeedWriteToFlash = g_tFlashSpecInterface.CheckPrgOrder(pFlash_phy, pDataBuf, pRedBuf, nType, INVALID_2F, pFlashCmdParam);
    }

    if (TRUE == bCheckErr && bNeedWriteToFlash == TRUE)
    {
        usErrCode = Flash_OpWrite((U8)pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);

        if (NF_SUCCESS == usErrCode && TRUE == pFlashCmdParam->module_inj_en)/*yanfei 20170407, Support P_ERR_INJ_DEC_REP Test*/
        {
           usErrCode= pFlashCmdParam->p_nf_cq_entry->bsErrTypeS;
           usErrSts = pFlashCmdParam->p_nf_cq_entry->bsInjStsVal;
        }
    }

    /* Update flash status related param */
    if (NF_SUCCESS == usErrCode)
    {
        /* Set and update pair page related parameter */
        FlashStsM_SetLpMpUpWrite(pFlash_phy);
        FlashStsM_SetPairPgSrcSeed(pFlash_phy, pFlashCmdParam->p_nf_cq_entry, ulSeedIndex);
        if(NULL != g_tFlashSpecInterface.IncPairPgPrgStep)
        {
            g_tFlashSpecInterface.IncPairPgPrgStep(pFlash_phy);
        }

        /* if pair page step equal PAIR_PAGE_PRG_MAX, set the ststus to close */
        if (PAIR_PAGE_PRG_MAX == FlashStsM_GetPairPgPrgStep(pFlash_phy) &&
            PAIR_PAGE_PROGRAM_FAIL != FlashStsM_GetPairPgSts(pFlash_phy))
        {
            FlashStsM_SetPairPgSts(pFlash_phy, PAIR_PAGE_CLOSE);
        }
    }

    /* Write data to flash */
    if (NF_SUCCESS == usErrCode && TRUE == bNeedWriteToFlash)
    {
        if (RSV_DATA_TYPE == nType)
        {
            Flash_Table_Handle(pFlash_phy, pDataBuf, pRedBuf, nLength, TRUE);
            Dbg_IncDevTableWriteCnt(1);

        }
        else if (COM_DATA_TYPE == nType)
        {
            Flash_Data_Handle(pFlash_phy, pDataBuf, pRedBuf, nLength, TRUE);
            Dbg_IncDevDataWriteCnt(1);
            if (0 == pFlash_phy->nPln)
            {
                Mid_FlashOpLog('w', pFlash_phy, (U32*)((U8*)pRedBuf + g_LPNInRedOffSet));
            }
        }
    }
    else if (NF_SUCCESS != usErrCode)
    {
        FlashStsM_SetPairPgSts (pFlash_phy, PAIR_PAGE_PROGRAM_FAIL);
        DBG_Printf("Lun %d Block %d Page: %d, PairPage: %d, Program fail!!\n", pFlash_phy->ucLunInTotal, pFlash_phy->nBlock, pFlash_phy->nPage,
            g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock)));
        usFstDsgPtr = pFlashCmdParam->p_nf_cq_entry->bsFstDsgPtr;
    }

    /* check pair page type when write cmd done */
    if(FALSE == FlashStsM_CheckPairPgType(pFlash_phy))
    {
        DBG_Getch();
    }

    /* Report Dec status */
    l_aPlaneErrType.ErrCode = usErrCode;
    l_aPlaneErrType.ErrSts = usErrSts;
    Flash_StsReport(tNfcOrgStruct, pFlashCmdParam, NFC_GetRP(tNfcOrgStruct), l_aPlaneErrType, pFlash_phy->nPln);

    return usErrCode;
}


void Flash_Data_Handle(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, UINT32 nDataLen, BOOL bWrite)
{
    FLASH_DATA_PLN *pDataPln = NULL;
    FLASH_DATA_PAGE *pDataPage = NULL;
    UINT64 *pTempBuf = (UINT64*)pDataBuf;
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->ucLunInTotal;
    UINT32 nSectorCnt = SEC_PER_PHYPG;
    UINT32 nSectorIndex = 0;
    UINT32 nU64CntPerSec = 512/8;
    UINT32 nRedSize = RED_SZ_DW;

    if (0 != (nDataLen % LOGIC_PG_SZ))
    {
        DBG_Printf("Flash_Write_Data: date length is must equal LOGIC_PG_SZ\n");
        DBG_Getch();
    }

    //if ((nPU == 0) && (nBlock == 0))
    //    DBG_Printf("is a error\n");
    pDataPln = g_FlashCOMData[nPU].pDataPln[nPln];
    pDataPage = &(pDataPln->DataPage[nBlock][nPage]);

    // write data
    if (bWrite)
    {
        if(NULL != pDataBuf)
        {
            for(nSectorIndex = 0; nSectorIndex < nSectorCnt; nSectorIndex++)
            {
                pDataPage->ComData[nSectorIndex].ulLBA =  ((FLASH_COM_DATA*)pTempBuf)->ulLBA;
                pDataPage->ComData[nSectorIndex].ulWriteCnt = ((FLASH_COM_DATA*)pTempBuf)->ulWriteCnt;
                pTempBuf += nU64CntPerSec;
            }
        }

        // write red data
        if(NULL != pRedBuf)
        {
            memcpy(pDataPage->RedData, pRedBuf, nRedSize*4);
        }
    }
    else
    {
        if(NULL != pDataBuf)
        {
            for(nSectorIndex = 0; nSectorIndex < nSectorCnt; nSectorIndex++)
            {
                ((FLASH_COM_DATA*)pTempBuf)->ulLBA = pDataPage->ComData[nSectorIndex].ulLBA;
                ((FLASH_COM_DATA*)pTempBuf)->ulWriteCnt = pDataPage->ComData[nSectorIndex].ulWriteCnt;
                pTempBuf += nU64CntPerSec;
            }
        }

        // read red data
        if(NULL != pRedBuf)
        {
            memcpy(pRedBuf, pDataPage->RedData,nRedSize*4);
        }
    }
}

UINT32 Flash_Table_FindEmptyPage(FLASH_TALBE_PAGE_MANT * pPageMntBase)
{
    UINT32 nPageIndex = 0;
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = pPageMntBase;
    for(nPageIndex = 0; nPageIndex < TABLE_PAGE_PER_PLN; nPageIndex++)
    {
        if (INVALID_4F == pCurPageMnt->bValid)
        {
            break;
        }
        pCurPageMnt++;
    }

    if (nPageIndex == TABLE_PAGE_PER_PLN)
        nPageIndex = INVALID_8F;
    return nPageIndex;
}

UINT32 Flash_Table_GetPageIndex(UINT32 nPagePhyID, UINT32 nStartIndex, FLASH_TALBE_PAGE_MANT *pPageMntBase)
{
    UINT32 nPageIndex = nStartIndex;
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = pPageMntBase;

    while (nPageIndex != INVALID_8F)
    {
        pCurPageMnt = pPageMntBase + nPageIndex;
        if (pCurPageMnt->PhyPageID == nPagePhyID)
        {
            break;
        }
        else
        {
            nPageIndex = pCurPageMnt->NextLogID;
        }
    }
    return nPageIndex;
}

UINT32 Flash_Table_SetNextLogID(UINT32 nLogID, UINT32 nNextLogID, FLASH_TALBE_PAGE_MANT *pPageMntBase)
{
    if (nLogID == 0xFFFF)
    {
        return 0;
    }

    FLASH_TALBE_PAGE_MANT *pCurPageMnt = pPageMntBase;
    pCurPageMnt = pPageMntBase + nLogID;
    pCurPageMnt->NextLogID = nNextLogID;

    return 0;
}

UINT32 Flash_Table_SetNewPageInfo(UINT32 nLogID,UINT32 nPhyPageID, FLASH_TALBE_PAGE_MANT *pPageMntBase)
{
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = pPageMntBase;
    pCurPageMnt = pPageMntBase + nLogID;
    pCurPageMnt->NextLogID = INVALID_8F;
    pCurPageMnt->bValid = 0;
    pCurPageMnt->PhyPageID = nPhyPageID;

    return 0;

}

UINT32 Flash_Talbe_GetPageType(PFLASH_PHY pFlash_phy)
{
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->ucLunInTotal;

    FLASH_TABLE_PLN *pTablePln = NULL;
    FLASH_TABLE_PAGE *pTablePage = NULL;
    FLASH_TALBE_PAGE_MANT *pPageMnt = NULL;
    FLASH_TALBE_BLOCK_MANT *pBlockMnt = NULL;
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = NULL;

    UINT32 nType = COM_DATA_TYPE;
    UINT32 nNextLogID = 0;

    pTablePln = g_FlashTableData[nPU].pTablePln[nPln];
    pPageMnt = (FLASH_TALBE_PAGE_MANT*)&(g_FlashTableData[nPU].pTablePln[nPln]->PageMnt);
    pBlockMnt = (FLASH_TALBE_BLOCK_MANT*)&(g_FlashTableData[nPU].pTablePln[nPln]->BlockMnt[nBlock]);

    if (pBlockMnt->StartLogPge != INVALID_4F)
    {
        pCurPageMnt = pPageMnt + pBlockMnt->StartLogPge;
        nNextLogID = pBlockMnt->StartLogPge;

        while(nNextLogID != INVALID_8F)
        {
            pCurPageMnt = pPageMnt + nNextLogID;
            if (pCurPageMnt->PhyPageID == nPage)
            {
                nType = RSV_DATA_TYPE;
                break;
            }
           nNextLogID = pCurPageMnt->NextLogID;
        }
    }

    return nType;
}

void Flash_Table_Handle(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, UINT32 nDataLen, BOOL bWrite)
{
    FLASH_TABLE_PLN *pTablePln = NULL;
    FLASH_TABLE_PAGE *pTablePage = NULL;
    FLASH_TALBE_PAGE_MANT *pPageMnt = NULL;
    FLASH_TALBE_BLOCK_MANT *pBlockMnt = NULL;

    UINT64 *pTempBuf = (UINT64*)pDataBuf;
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->ucLunInTotal;
    UINT32 nSectorCnt = SEC_PER_PHYPG;
    UINT32 nSectorIndex = 0;
    UINT32 nU64CntPerSec = 512/sizeof(UINT64);
    UINT32 nRedSize = RED_SZ_DW;
    UINT32 nPageIndex = INVALID_8F;

    if (0 != (nDataLen % LOGIC_PG_SZ))
    {
        DBG_Printf("Flash_Write_Data: date length is must equal LOGIC_PG_SZ\n");
        DBG_Getch();
    }

    pTablePln = g_FlashTableData[nPU].pTablePln[nPln];
    pPageMnt = (FLASH_TALBE_PAGE_MANT*)&(g_FlashTableData[nPU].pTablePln[nPln]->PageMnt);
    pBlockMnt = (FLASH_TALBE_BLOCK_MANT*)&(g_FlashTableData[nPU].pTablePln[nPln]->BlockMnt[nBlock]);

    if (bWrite)
    {
        // write data
        nPageIndex = Flash_Table_FindEmptyPage(pPageMnt);

        if (nPageIndex == INVALID_8F)
        {
            DBG_Printf("Flash_Table_Handle: there have no free page\n");
            DBG_Printf("Pu %d Pln %d Blk %d Pg %d\n", nPU, nPln, nBlock, nPage);
            DBG_Getch();
        }
        pTablePage = &(pTablePln->PageData[nPageIndex]);
        memcpy(pTablePage->TableData, pDataBuf, LOGIC_PG_SZ);
        // write red data
        memcpy(pTablePage->RedData, pRedBuf, nRedSize*4);

        if (pBlockMnt->StartLogPge == INVALID_4F)
        {
            pBlockMnt->StartLogPge = nPageIndex;
        }

        Flash_Table_SetNextLogID(pBlockMnt->EndLogPge, nPageIndex, pPageMnt);
        Flash_Table_SetNewPageInfo(nPageIndex, nPage,pPageMnt);
        pBlockMnt->EndLogPge = nPageIndex;

    }
    else
    {
        nPageIndex = Flash_Table_GetPageIndex(nPage, pBlockMnt->StartLogPge, pPageMnt);
        pTablePage = &(pTablePln->PageData[nPageIndex]);

        if (nPageIndex == INVALID_8F)
        {
            DBG_Printf("Flash_Table_Handle: there have no free page\n");
            DBG_Getch();
        }
        // read data
        if (NULL != pDataBuf)
        {
            memcpy(pDataBuf, pTablePage->TableData, LOGIC_PG_SZ);
        }
        // read red data
        if (NULL != pRedBuf)
        {
            memcpy(pRedBuf, pTablePage->RedData,nRedSize*4);
        }
    }
}

BOOL Flash_TlcPrgDataCmp(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nType)
{
    UINT64 *pTempBuf = (UINT64*)pDataBuf;
    UINT64 *pRedTempBuf = (UINT64*)pRedBuf;
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->ucLunInTotal;
    UINT32 nSectorCnt = SEC_PER_PHYPG;
    UINT32 nSectorIndex = 0;
    UINT32 nU64CntPerSec = 512 / sizeof(UINT64);
    UINT32 nRedSize = RED_SZ_DW;
    UINT32 nPageIndex = INVALID_8F;
    UINT32 nDwIndex = 0;

    if (nType == COM_DATA_TYPE)
    {
        FLASH_DATA_PLN *pDataPln = NULL;
        FLASH_DATA_PAGE *pDataPage = NULL;

        pDataPln = g_FlashCOMData[nPU].pDataPln[nPln];
        pDataPage = &(pDataPln->DataPage[nBlock][nPage]);

        /*Compare Red*/
        if ((U32)*pRedTempBuf != (U32)*pDataPage->RedData)
        {
            DBG_Printf("DataRed Compare with first program data are different\n");
            return FALSE;
        }

        /*Compare Data*/
        for (nSectorIndex = 0; nSectorIndex < nSectorCnt; nSectorIndex++)
        {
            //DBG_Printf("pDataBuf->LBA:%x, pDataBuf->WriteCnt:%x\n", ((FLASH_COM_DATA*)pTempBuf)->ulLBA, ((FLASH_COM_DATA*)pTempBuf)->ulWriteCnt);
            //DBG_Printf("pDataPage->LBA:%x, pDataPage->WriteCnt:%x\n", pDataPage->ComData[nSectorIndex].ulLBA, pDataPage->ComData[nSectorIndex].ulWriteCnt);

            if (((FLASH_COM_DATA*)pTempBuf)->ulLBA != pDataPage->ComData[nSectorIndex].ulLBA ||
                ((FLASH_COM_DATA*)pTempBuf)->ulWriteCnt != pDataPage->ComData[nSectorIndex].ulWriteCnt)
            {
                DBG_Printf("Data Compare with first program data are different!\n");
                return FALSE;
            }
            pTempBuf += nU64CntPerSec;
        }
    }
    else
    {
        FLASH_TABLE_PLN *pTablePln = NULL;
        FLASH_TABLE_PAGE *pTablePage = NULL;
        FLASH_TALBE_PAGE_MANT *pPageMnt = NULL;
        FLASH_TALBE_BLOCK_MANT *pBlockMnt = NULL;

        pTablePln = g_FlashTableData[nPU].pTablePln[nPln];
        pPageMnt = (FLASH_TALBE_PAGE_MANT*)&(g_FlashTableData[nPU].pTablePln[nPln]->PageMnt);
        pBlockMnt = (FLASH_TALBE_BLOCK_MANT*)&(g_FlashTableData[nPU].pTablePln[nPln]->BlockMnt[nBlock]);

        nPageIndex = Flash_Table_GetPageIndex(nPage, pBlockMnt->StartLogPge, pPageMnt);
        pTablePage = &(pTablePln->PageData[nPageIndex]);

        //DBG_Printf("pDataBuf:%x\n", *pTempBuf);
        //DBG_Printf("pTablePage->TableData:%x\n", *pTablePage->TableData);

        /*Compare Red*/
        if ((U32)*pRedTempBuf != (U32)*pTablePage->RedData)
        {
            DBG_Printf("TableRed Compare with first program data are different!\n");
            return FALSE;
        }

        /*Compare Data*/
        if ((U32)*pTempBuf != (U32)*pTablePage->TableData)
        {
            DBG_Printf("Table Compare with first program data are different!\n");
            return FALSE;
        }
    }

    return TRUE;
}

void Mid_CopyTableData(PFLASH_PHY pFlash_phy_source, PFLASH_PHY pFlash_phy_target, BOOL bsTLCMode)
{
    FLASH_TABLE_PLN *pTablePlnSource = NULL;
    FLASH_TABLE_PAGE *pTablePageSource = NULL;
    FLASH_TALBE_PAGE_MANT *pPageMntSource = NULL;
    FLASH_TALBE_BLOCK_MANT *pBlockMntSource = NULL;
    UINT32 nSourceBlock = pFlash_phy_source->nBlock;
    UINT32 nSourcePage = pFlash_phy_source->nPage;
    UINT32 nSourcePln = pFlash_phy_source->nPln;
    UINT32 nSourcePU = pFlash_phy_source->ucLunInTotal;

    FLASH_TABLE_PLN *pTablePlnTarget = NULL;
    FLASH_TABLE_PAGE *pTablePageTarget = NULL;
    FLASH_TALBE_PAGE_MANT *pPageMntTarget = NULL;
    FLASH_TALBE_BLOCK_MANT *pBlockMntTarget = NULL;
    UINT32 nTargetBlock = pFlash_phy_target->nBlock;
    UINT32 nTargetPage = pFlash_phy_target->nPage;
    UINT32 nTargetPln = pFlash_phy_target->nPln;
    UINT32 nTargetPU = pFlash_phy_target->ucLunInTotal;

    UINT32 nSectorCnt = SEC_PER_PHYPG;
    UINT32 nSectorIndex = 0;
    UINT32 nU64CntPerSec = 512 / sizeof(UINT64);
    UINT32 nRedSize = RED_SZ_DW;
    UINT32 nSourcePageIndex = INVALID_8F;
    UINT32 nTargetPageIndex = INVALID_8F;

    pTablePlnSource = g_FlashTableData[nSourcePU].pTablePln[nSourcePln];
    pPageMntSource = (FLASH_TALBE_PAGE_MANT*)&(g_FlashTableData[nSourcePU].pTablePln[nSourcePln]->PageMnt);
    pBlockMntSource = (FLASH_TALBE_BLOCK_MANT*)&(g_FlashTableData[nSourcePU].pTablePln[nSourcePln]->BlockMnt[nSourceBlock]);

    pTablePlnTarget = g_FlashTableData[nTargetPU].pTablePln[nTargetPln];
    pPageMntTarget = (FLASH_TALBE_PAGE_MANT*)&(g_FlashTableData[nTargetPU].pTablePln[nTargetPln]->PageMnt);
    pBlockMntTarget = (FLASH_TALBE_BLOCK_MANT*)&(g_FlashTableData[nTargetPU].pTablePln[nTargetPln]->BlockMnt[nTargetBlock]);

    // write data
    nTargetPageIndex = Flash_Table_FindEmptyPage(pPageMntTarget);

    if (nTargetPageIndex == INVALID_8F)
    {
        DBG_Printf("Flash_Table_Handle: there have no free page\n");
        DBG_Printf("Pu %d Pln %d Blk %d Pg %d\n", nTargetPU, nTargetPln, nTargetBlock, nTargetPage);
        DBG_Getch();
    }

    // read data
    nSourcePageIndex = Flash_Table_GetPageIndex(nSourcePage, pBlockMntSource->StartLogPge, pPageMntSource);

    pTablePageSource = &(pTablePlnSource->PageData[nSourcePageIndex]);
    pTablePageTarget = &(pTablePlnTarget->PageData[nTargetPageIndex]);


    memcpy(pTablePageTarget->TableData, pTablePageSource->TableData, LOGIC_PG_SZ);
    // write red data
    memcpy(pTablePageTarget->RedData, pTablePageSource->RedData, nRedSize * 4);

    if (pBlockMntTarget->StartLogPge == INVALID_4F)
    {
        pBlockMntTarget->StartLogPge = nTargetPageIndex;
    }

    Flash_Table_SetNextLogID(pBlockMntTarget->EndLogPge, nTargetPageIndex, pPageMntTarget);
    Flash_Table_SetNewPageInfo(nTargetPageIndex, nTargetPage, pPageMntTarget);
    pBlockMntTarget->EndLogPge = nTargetPageIndex;

    if (BLOCK_INVALID == FlashStsM_GetBlkType(pFlash_phy_target->ucLunInTotal, pFlash_phy_target->nPln, pFlash_phy_target->nBlock) ||
        BLOCK_TLC_LOWPG == FlashStsM_GetBlkType(pFlash_phy_target->ucLunInTotal, pFlash_phy_target->nPln, pFlash_phy_target->nBlock))
    {
        if (g_tFlashSpecInterface.SetBlkType != NULL)
        {
            g_tFlashSpecInterface.SetBlkType(pFlash_phy_target->ucLunInTotal, pFlash_phy_target->nPln, pFlash_phy_target->nBlock, pFlash_phy_target->nPage, CMD_CODE_PROGRAM, bsTLCMode);
        }
        else
        {
            DBG_Printf("Mid_CopyTableData: SetBlkType interface NULL!! \n");
            DBG_Getch();
        }
    }
}

U8 Mid_CopyData(PFLASH_PHY pFlash_phy_source, PFLASH_PHY pFlash_phy_target, BOOL bsTLCMode)
{
    FLASH_DATA_PLN *pDataPlnSource = NULL;
    FLASH_DATA_PAGE *pDataPageSource = NULL;
    UINT32 nSourceBlock = pFlash_phy_source->nBlock;
    UINT32 nSourcePage = pFlash_phy_source->nPage;
    UINT32 nSourcePln = pFlash_phy_source->nPln;
    UINT32 nSourcePU = pFlash_phy_source->ucLunInTotal;

    FLASH_DATA_PLN *pDataPlnTarget = NULL;
    FLASH_DATA_PAGE *pDataPageTarget = NULL;
    UINT32 nTaegetBlock = pFlash_phy_target->nBlock;
    UINT32 nTaegetPage = pFlash_phy_target->nPage;
    UINT32 nTaegetPln = pFlash_phy_target->nPln;
    UINT32 nTaegetPU = pFlash_phy_target->ucLunInTotal;

    UINT32 nSectorCnt = SEC_PER_PHYPG;
    UINT32 nSectorIndex = 0;
    UINT32 nU64CntPerSec = 512 / 8;
    UINT32 nRedSize = RED_SZ_DW;

    U8 ucErrCode;
    ucErrCode = Flash_OpWrite((U8)pFlash_phy_target->ucLunInTotal, pFlash_phy_target->nPln, pFlash_phy_target->nBlock, pFlash_phy_target->nPage);
    if (NF_SUCCESS != ucErrCode) return ucErrCode;

    pDataPlnSource = g_FlashCOMData[nSourcePU].pDataPln[nSourcePln];
    pDataPageSource = &(pDataPlnSource->DataPage[nSourceBlock][nSourcePage]);

    pDataPlnTarget = g_FlashCOMData[nTaegetPU].pDataPln[nTaegetPln];
    pDataPageTarget = &(pDataPlnTarget->DataPage[nTaegetBlock][nTaegetPage]);

    // write data
    for (nSectorIndex = 0; nSectorIndex < nSectorCnt; nSectorIndex++)
    {
        pDataPageTarget->ComData[nSectorIndex].ulLBA = pDataPageSource->ComData[nSectorIndex].ulLBA;
        pDataPageTarget->ComData[nSectorIndex].ulWriteCnt = pDataPageSource->ComData[nSectorIndex].ulWriteCnt;
    }

    // write red data
    memcpy(pDataPageTarget->RedData, pDataPageSource->RedData, nRedSize * 4);

    if (BLOCK_INVALID == FlashStsM_GetBlkType(pFlash_phy_target->ucLunInTotal, pFlash_phy_target->nPln, pFlash_phy_target->nBlock) ||
        BLOCK_TLC_LOWPG == FlashStsM_GetBlkType(pFlash_phy_target->ucLunInTotal, pFlash_phy_target->nPln, pFlash_phy_target->nBlock))
    {
        if (g_tFlashSpecInterface.SetBlkType != NULL)
        {
            g_tFlashSpecInterface.SetBlkType(pFlash_phy_target->ucLunInTotal, pFlash_phy_target->nPln, pFlash_phy_target->nBlock, pFlash_phy_target->nPage, CMD_CODE_PROGRAM, bsTLCMode);
        }
        else
        {
            DBG_Printf("Mid_CopyData: SetBlkType interface NULL!! \n");
            DBG_Getch();
        }
    }

    return NF_SUCCESS;
}

U32 Flash_GetBlockType(U8 usCE, U16 uBlock)
{

    U32 ulPageType = 0;
    FLASH_PHY FlashAdd = { 0 };

    FlashAdd.nPage = 0;
    FlashAdd.nBlock = uBlock;
    FlashAdd.ucLunInTotal = usCE;

    ulPageType = Flash_Talbe_GetPageType(&FlashAdd);

    return ulPageType;
}

// return an error code
U8 Mid_Read(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nDataLen, BOOL bCheckErr, ST_FLASH_CMD_PARAM *pFlashCmdParam, const NFCM_LUN_LOCATION *tNfcOrgStruct, U32 ulSeedIndex)
{
    UINT32 nPageType = Flash_Talbe_GetPageType(pFlash_phy);
    U8 usErrCode = NF_SUCCESS;
    U8 usDWIndex = 0;
    U32 *pDWAddr = (U32 *)pRedBuf;
    U8 ucCmdType = pFlashCmdParam->bsCmdType;
    BOOL bsTLCMode = pFlashCmdParam->bsIsTlcMode;
    PLANE_ERR_TYPE l_aPlaneErrType;
    BOOL bIsEmptyErr = FALSE;
    SIM_NFC_RED * pRed;
    BOOL bsRawReadDEn = pFlashCmdParam->p_nf_cq_entry->bsRawReadEn;

    /* Check cmd mode */
    if (FlashstsM_GetLunMode(pFlash_phy->ucLunInTotal) != NFC_GetCmdMode(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct)))
    {
        DBG_Printf("Read command work mode error\n");
        DBG_Printf("PU %d LUN %d Blk %d Page %d\r\n", pFlash_phy->ucLunInTotal, pFlash_phy->ucLunInTotal, pFlash_phy->nBlock,pFlash_phy->nPage);
        DBG_Printf("LunMode: %d, CmdMode: %d\n", FlashstsM_GetLunMode(pFlash_phy->ucLunInTotal), NFC_GetCmdMode(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct)));
        DBG_Getch();
    }

    /* check block erased fail */
    if(TRUE == FlashStsM_CheckEraseFail(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock))
    {
        DBG_Printf("Read erased fail block !! LunInTotal%d Blk%d Pln%d\n", pFlash_phy->ucLunInTotal, pFlash_phy->nBlock, pFlash_phy->nPln);
        DBG_Getch();
    }

    /*check read scramble seed if write */
    if(TRUE == FlashStsM_GetLpMpUpWrite(pFlash_phy))
    {
        /* SoftDec use raw read cmd, so don't check scramble seed */
        if (TRUE != bsRawReadDEn)
        {
            if (FALSE == FlashStsM_CheckPairPgSrcSeed(FlashStsM_GetPairPgSrcSeed(pFlash_phy), pFlashCmdParam->p_nf_cq_entry, ulSeedIndex))
            {
                DBG_Printf("PU %d, Pln %d, Blk %d, Pg %d, Read scramble seed error!\n", pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
                DBG_Getch();
            }
        }
    }

    if (TRUE == bCheckErr)
    {
        usErrCode = Flash_OpRead((U8)pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
    }

    if (NF_SUCCESS == usErrCode && g_tFlashSpecInterface.ReadCheck != NULL)
    {
        usErrCode = g_tFlashSpecInterface.ReadCheck(pFlash_phy);
    }

    /* check block type */
    if(g_tFlashSpecInterface.CheckBlkType != NULL &&
        FALSE == g_tFlashSpecInterface.CheckBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, ucCmdType, bsTLCMode))
    {
        DBG_Printf("Read cmd: check Block type error, Lun %d, Pln %d, Block %d\n", pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock);
        usErrCode = NF_ERR_TYPE_UECC;
    }

    /* check read program fail pair page */
    if(TRUE == FlashStsM_CheckPrgFail(pFlash_phy))
    {
        DBG_Printf("Read cmd: read progam failed pair page, Lun %d, Pln %d, Block %d, Page %d\n", pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
        usErrCode = NF_ERR_TYPE_UECC;
    }

#if (defined(FLASH_TLC) && !defined(FLASH_TSB_3D))
    /* check next open WL issue */
    if (TRUE == FlashStsM_NextWLOpenCheck(pFlash_phy) && TRUE == bsTLCMode)
    {
        DBG_Printf("Read cmd: WL %d's next WL is opened\n", g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock)));
        usErrCode = NF_ERR_TYPE_UECC;
    }
#endif

    //have no err code then read data
    if (RSV_DATA_TYPE == nPageType)
    {
        Flash_Table_Handle(pFlash_phy, pDataBuf, pRedBuf, nDataLen, FALSE);
        Dbg_IncDevTableReadCnt(1);
    }
    else if (COM_DATA_TYPE == nPageType)
    {
        Flash_Data_Handle(pFlash_phy, pDataBuf, pRedBuf, nDataLen, FALSE);
        Dbg_IncDevDataReadCnt(1);
        if (0 == pFlash_phy->nPln)
        {
            Mid_FlashOpLog('r', pFlash_phy, (U32*)((U8*)pRedBuf + g_LPNInRedOffSet));
        }
    }

    // UECC > empty page > RECC
    //
    for (usDWIndex = 0; usDWIndex < RED_SZ_DW; usDWIndex++)
    {
        if (*pDWAddr != INVALID_8F)
        {
            break;
        }
        pDWAddr++;
    }

    if (usDWIndex == RED_SZ_DW)
    {
        if (NF_ERR_TYPE_UECC == usErrCode)
        {
            Flash_SetMaxRetryTime((U8)pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
        }
        else
        {
            /// @todo usErrCode = NF_ERR_TYPE_EMPTY_PG;
            usErrCode = NF_ERR_TYPE_UECC; // need to recheck it in fw errhandling when empty page
        }
    }

    /* update read statistic info */
    FlashStsM_AddOpenBlkRdCnt(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock);
    FlashStsM_AddOpenWLRdCnt(pFlash_phy);
    FlashStsM_AddNextOpenWLRdCnt(pFlash_phy);

    /* Report Dec status */
    if (NF_SUCCESS != usErrCode)
    {
        U8 ucPageType;
        pRed = ((SIM_NFC_RED *)pRedBuf);
        ucPageType = pRed->m_RedComm.bcPageType;
        bIsEmptyErr = (PG_TYPE_FREE == ucPageType) ? TRUE : FALSE;
    }

    /* if cmd error injection opened, report status */
    if(TRUE == pFlashCmdParam->inj_en &&  FALSE == pFlashCmdParam->p_nf_cq_entry->bsRedOnly)
    {
        usErrCode = (pFlashCmdParam->p_nf_cq_entry->bsInjErrCnt > CW_ERR_CNT_THRESHOLD) ? NF_ERR_TYPE_UECC: NF_ERR_TYPE_RECC;
    }

    if(TRUE == pFlashCmdParam->inj_en &&  TRUE == pFlashCmdParam->p_nf_cq_entry->bsRedOnly)
    {
        usErrCode = (pFlashCmdParam->p_nf_cq_entry->bsInjErrCnt > RED_CW_ERR_CNT_THRESHOLD) ? NF_ERR_TYPE_UECC: NF_ERR_TYPE_RECC;
    }

    l_aPlaneErrType.ErrCode = usErrCode;
    l_aPlaneErrType.IsEmptyPG = bIsEmptyErr;

    usErrCode = Flash_StsReport(tNfcOrgStruct, pFlashCmdParam, NFC_GetRP(tNfcOrgStruct), l_aPlaneErrType, pFlash_phy->nPln);

    //SystemStatisticRecord("Mid_Read: CE = %d, block = %d, page = %d, errorcode = %d\n", pFlash_phy->nPU, pFlash_phy->nBlock, pFlash_phy->nPage, usErrCode);

    return usErrCode;
}

// return error code;
U8 Mid_Erase(PFLASH_PHY pFlash_phy, BOOL bCheckErr, BOOL bsLLF, ST_FLASH_CMD_PARAM *pFlashCmdParam, const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->ucLunInTotal;
    FLASH_TABLE_PLN *pTablePln = NULL;
    FLASH_TABLE_PAGE *pTablePage = NULL;
    FLASH_DATA_PLN *pDataPln = NULL;
    FLASH_DATA_PAGE *pDataPage = NULL;
    FLASH_TALBE_PAGE_MANT *pPageMnt = NULL;
    FLASH_TALBE_BLOCK_MANT *pBlockMnt = NULL;
    UINT32 nNextLogID = 0;
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = NULL;
    BOOL bError = FALSE;
    U8 usErrCode = 0;
    PLANE_ERR_TYPE l_aPlaneErrType;

#if 0
    // for flash test, disable the block type check
    if (TRUE != bsLLF)
    {
        if(g_tFlashSpecInterface.CheckBlkType != NULL &&
           FALSE == g_tFlashSpecInterface.CheckBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, ucCmdType, bsTLCMode))
        {
            DBG_Printf("The cmd can't operate the block type!!\n");
            DBG_Getch();
        }
    }
#endif

    /* Check cmd mode */
    if (FlashstsM_GetLunMode(nPU) != NFC_GetCmdMode(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct)))
    {
        DBG_Printf("Erase command work mode error\n");
        DBG_Printf("LunMode: %d, CmdMode: %d\n", FlashstsM_GetLunMode(nPU), NFC_GetCmdMode(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct)));
        DBG_Getch();
    }

    //bError = Flash_UpdateEraseCnt((U8)nPU, (U16)nBlock);
    if (TRUE == bCheckErr)
    {
        /* check block type match cmd code */
        if(NULL != g_tFlashSpecInterface.CheckErsType &&
            FALSE == g_tFlashSpecInterface.CheckErsType(nPU, nPln, nBlock, pFlashCmdParam->bsCmdCode))
        {
            DBG_Printf("Flash_MidErase: CEIndex = %d, PlnIndex = %d, BlockIndex = %d, Erase block type error\n", nPU, nPln, nBlock);
            DBG_Getch();
        }

        usErrCode = Flash_OpErase(nPU, nPln, nBlock, bsLLF);

        /* check block and its pair page status free */
        if (NF_SUCCESS == usErrCode && FALSE == FlashStsM_CheckStsFree(nPU, nPln, nBlock))
        {
            DBG_Printf("Flash_MidErase: CEIndex = %d, PlnIndex = %d, BlockIndex = %d, Status free check error\n", nPU, nPln, nBlock);
            DBG_Getch();
        }

    }

    if(NF_SUCCESS == usErrCode)
    {
        // clear data page
        pDataPln = g_FlashCOMData[nPU].pDataPln[nPln];
        pDataPage = &(pDataPln->DataPage[nBlock][0]);
        memset(pDataPage, 0xFF, sizeof(FLASH_DATA_PAGE)*LOGIC_PG_PER_BLK);

        // clear table page
        pTablePln = g_FlashTableData[nPU].pTablePln[nPln];
        pPageMnt = (FLASH_TALBE_PAGE_MANT*)&(pTablePln->PageMnt);
        pBlockMnt = (FLASH_TALBE_BLOCK_MANT*)&(pTablePln->BlockMnt[nBlock]);
        if (pBlockMnt->StartLogPge != INVALID_4F)
        {
            pCurPageMnt = pPageMnt + pBlockMnt->StartLogPge;
            nNextLogID = pBlockMnt->StartLogPge;

            while(nNextLogID != INVALID_8F)
            {
                pCurPageMnt = pPageMnt + nNextLogID;
                pTablePage = &(pTablePln->PageData[nNextLogID]);

            memset(pTablePage, 0xFF, LOGIC_PG_SZ);
            nNextLogID = pCurPageMnt->NextLogID;

                pCurPageMnt->bValid = 0xFFFF;
                pCurPageMnt->NextLogID = INVALID_8F;
                pCurPageMnt->PhyPageID = INVALID_4F;
            }

            pBlockMnt->EndLogPge = INVALID_4F;
            pBlockMnt->StartLogPge = INVALID_4F;
        }
    }

    /* Report Dec status */
    l_aPlaneErrType.ErrCode = usErrCode;
    l_aPlaneErrType.ErrSts = pFlashCmdParam->p_nf_cq_entry->bsInjStsVal;
    Flash_StsReport(tNfcOrgStruct, pFlashCmdParam, NFC_GetRP(tNfcOrgStruct), l_aPlaneErrType, nPln);

    return usErrCode;
}

void Mid_Read_RedData(PFLASH_PHY pFlash_phy, char *pRedBuf)
{
    UINT32 nPageType = Flash_Talbe_GetPageType(pFlash_phy);

    if (RSV_DATA_TYPE == nPageType)
    {
        Flash_Table_Handle(pFlash_phy, NULL, pRedBuf, LOGIC_PG_SZ, FALSE);
    }
    else if (COM_DATA_TYPE == nPageType)
    {
        Flash_Data_Handle(pFlash_phy, NULL, pRedBuf, LOGIC_PG_SZ, FALSE);
    }
}

U8 Mid_Read_FlashIDB(PFLASH_PHY pFlash_phy)
{
    BOOL bBadBlock = FALSE;
    U8 usReturnValue = NORMAL_BLK_MARK;

    bBadBlock = FlashStsM_CheckEraseFail((U8)pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock);
    if (TRUE == bBadBlock)
    {
        usReturnValue = BAD_BLK_MARK;
    }

    return usReturnValue;
}
U8 Mid_GetFlashErrCode(PFLASH_PHY pFlash_phy)
{
    return Flash_GetErrorCode(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
}

void Mid_SearchLba(U32 ulLba)
{
    FLASH_DATA_PLN *pDataPln = NULL;
    FLASH_DATA_PAGE *pDataPage = NULL;
    U32 ulSectorIndex;
    U32 ulDataPageIndex;
    U32 ulBlockIndex;
    U32 ulSavedLba = 0;
    U8 uPuIndex;
    U8 uPlnIndex;


    for (uPuIndex = 0; uPuIndex < NFC_MODEL_LUN_SUM; uPuIndex++)
    {
        for (uPlnIndex = 0; uPlnIndex < 2; uPlnIndex++)
        {
            pDataPln = g_FlashCOMData[uPuIndex].pDataPln[uPlnIndex];
            for(ulBlockIndex = 0; ulBlockIndex < TOTAL_BLOCK_PER_PLN; ulBlockIndex++)
            {
                for (ulDataPageIndex = 0; ulDataPageIndex < LOGIC_PG_PER_BLK; ulDataPageIndex++)
                {
                    pDataPage = &(pDataPln->DataPage[ulBlockIndex][ulDataPageIndex]);
                    for (ulSectorIndex = 0; ulSectorIndex < SEC_PER_PHYPG; ulSectorIndex++)
                    {
                        ulSavedLba = pDataPage->ComData[ulSectorIndex].ulLBA;
                        if (ulLba == ulSavedLba)
                        {
                            DBG_Printf("Find Lba 0x%x at PU = %d, Pln = %d, Block = %d, Page = %d, SecIndex = %d, WriteCnt = %d \n",
                                       ulLba,uPuIndex, uPlnIndex, ulBlockIndex, ulDataPageIndex, ulSectorIndex,
                                        pDataPage->ComData[ulSectorIndex].ulWriteCnt);
                        }
                    }
                }
            }
        }

    }


    return;
}

void FlashSpecInterfaceFactory(void)
{
#if defined(FLASH_TLC)
    # if defined(FLASH_TSB_3D)
      Tsb3dTlc_InterfaceInit(&g_tFlashSpecInterface);
    # else
      Tsb2dTlc_InterfaceInit(&g_tFlashSpecInterface);
    # endif
#else
    # if defined(FLASH_L95)
      L95_InterfaceInit(&g_tFlashSpecInterface);
    # elif defined(FLASH_TSB)
      Tsb4Pln_InterfaceInit(&g_tFlashSpecInterface);
    # elif defined (FLASH_INTEL_3DTLC)
        # if defined (FLASH_IM_3DTLC_GEN2)
          IM3dTlcGen2_InterfaceInit(&g_tFlashSpecInterface);
        # else
          Intel3dTlc_InterfaceInit(&g_tFlashSpecInterface);
        # endif
    # endif
#endif

  return;
}
