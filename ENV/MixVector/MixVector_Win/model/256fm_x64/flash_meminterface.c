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
#include "win_bootloader.h"

#define SATA_MAX_SECTION            (((MAX_LPN_IN_SYSTEM << SEC_PER_LPN_BITS) / SATA_LBA_PER_SECTION) + (((MAX_LPN_IN_DISK_MAX << SEC_PER_LPN_BITS)%SATA_LBA_PER_SECTION)? 1:0))

#define SATA_LBA_IN_SECTION(ulLBA)  (ulLBA/SATA_LBA_PER_SECTION)

SATA_RAM_DEF g_SATARamData[SATA_MAX_SECTION] = {0};
FLASH_COM_PU_DEF g_FlashCOMData[CE_SUM] = {0};
FLASH_TABLE_PU_DEF g_FlashTableData[CE_SUM] = {0};

void Flash_Data_Handle(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, UINT32 nDataLen, BOOL bWrite);
void Flash_Table_Handle(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, UINT32 nDataLen, BOOL bWrite);

FLASH_OP_LOG l_FlashOpLog[CE_SUM] = {0};

#if  defined(L1_FAKE) || defined(L2_FAKE)
U8 g_LPNInRedOffSet = 0x10;
#else
extern U8 g_LPNInRedOffSet;
#endif

void Mid_FlashOpLog(char op, PFLASH_PHY pFlash_phy, U32 *pLpnAddr)
{
    U16 uCE = pFlash_phy->nPU;
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
    char strLogFileName[32];
    U32 ulLogIndex = 0;
    U32 ulNextLogIndex = 0;
    U16 usCEIndex = 0;
    U32 ulLoopIndex = 0;

    for (usCEIndex = 0; usCEIndex < CE_SUM; usCEIndex++)
    {
        PFLASH_PHY pFlashPhy;
        sprintf(strLogFileName, "FlashOpLog_%d.txt", usCEIndex);
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
                    ulLogIndex, pFlashLog->FlashAddr.nPU, pFlashLog->FlashAddr.nBlock, pFlashLog->FlashAddr.nPage, pFlashLog->FlashOP);
                
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
    UINT32 nPu = CE_SUM;
    UINT32 nPuIndex = 0;
    UINT32 nDataBlockPerPln = DATA_BLOCK_PER_PLN;
    UINT32 nTableBlockPerPln = NORMAL_BLOCK_ADDR_BASE;
    UINT32 nPagePerBlock = PG_PER_BLK;
    UINT32 nPlnIndex = 0;
    UINT32 nPlnPerPu = PLN_PER_PU;
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

void Mid_Write(PFLASH_PHY pFlash_phy, int nType, char* pDataBuf, char * pRedBuf, int nLength)
{

    if (RSV_DATA_TYPE == nType)
    {
        Flash_Table_Handle(pFlash_phy, pDataBuf, pRedBuf, nLength, TRUE);
    }
    else if (COM_DATA_TYPE == nType)
    {
        Flash_Data_Handle(pFlash_phy, pDataBuf, pRedBuf, nLength, TRUE);
        if (0 == pFlash_phy->nPln)
        {
            Mid_FlashOpLog('w', pFlash_phy, (U32*)((U8*)pRedBuf + g_LPNInRedOffSet));
        }
    }

}

void Flash_Data_Handle(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, UINT32 nDataLen, BOOL bWrite)
{
    FLASH_DATA_PLN *pDataPln = NULL;
    FLASH_DATA_PAGE *pDataPage = NULL;
    UINT64 *pTempBuf = (UINT64*)pDataBuf;
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->nPU;
    UINT32 nSectorCnt = SEC_PER_PG;
    UINT32 nSectorIndex = 0;
    UINT32 nU64CntPerSec = 512/8; 
    UINT32 nRedSize = RED_SZ_DW;

    if (0 != (nDataLen % PG_SZ))
    {
        DBG_Printf("Flash_Write_Data: date length is must equal PG_SZ\n");
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
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = pPageMntBase;
    pCurPageMnt = pPageMntBase + nLogID;
    pCurPageMnt->NextLogID = nNextLogID;
    // by Charles Zhou
    return 0;
}

UINT32 Flash_Table_SetNewPageInfo(UINT32 nLogID,UINT32 nPhyPageID, FLASH_TALBE_PAGE_MANT *pPageMntBase)
{
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = pPageMntBase;
    pCurPageMnt = pPageMntBase + nLogID;
    pCurPageMnt->NextLogID = INVALID_8F;
    pCurPageMnt->bValid = 0;
    pCurPageMnt->PhyPageID = nPhyPageID;
    // by Charles Zhou
    return 0;

}

UINT32 Flash_Talbe_GetPageType(PFLASH_PHY pFlash_phy)
{
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->nPU;

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
    UINT32 nPU = pFlash_phy->nPU;
    UINT32 nSectorCnt = SEC_PER_PG;
    UINT32 nSectorIndex = 0;
    UINT32 nU64CntPerSec = 512/sizeof(UINT64); 
    UINT32 nRedSize = RED_SZ_DW;
    UINT32 nPageIndex = INVALID_8F;

    if (0 != (nDataLen % PG_SZ))
    {
        DBG_Printf("Flash_Write_Data: date length is must equal PG_SZ\n");
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
            DBG_Getch();
        }
        pTablePage = &(pTablePln->PageData[nPageIndex]);
        memcpy(pTablePage->TableData, pDataBuf, PG_SZ);
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
            memcpy(pDataBuf, pTablePage->TableData, PG_SZ);
        }
        // read red data
        if (NULL != pRedBuf)
        {
            memcpy(pRedBuf, pTablePage->RedData,nRedSize*4);
        }
    }
}

void Mid_Read(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nDataLen)
{
    
    UINT32 nPageType = Flash_Talbe_GetPageType(pFlash_phy);

    if (RSV_DATA_TYPE == nPageType)
    {
        Flash_Table_Handle(pFlash_phy, pDataBuf, pRedBuf, nDataLen, FALSE);
    }
    else if (COM_DATA_TYPE == nPageType)
    {
        Flash_Data_Handle(pFlash_phy, pDataBuf, pRedBuf, nDataLen, FALSE);
        if (0 == pFlash_phy->nPln)
        {
            Mid_FlashOpLog('r', pFlash_phy, (U32*)((U8*)pRedBuf + g_LPNInRedOffSet));
        }
    }
}

void Mid_Erase(PFLASH_PHY pFlash_phy)
{
    UINT32 nBlock = pFlash_phy->nBlock;
    UINT32 nPage = pFlash_phy->nPage;
    UINT32 nPln = pFlash_phy->nPln;
    UINT32 nPU = pFlash_phy->nPU;
    FLASH_TABLE_PLN *pTablePln = NULL;
    FLASH_TABLE_PAGE *pTablePage = NULL;
    FLASH_DATA_PLN *pDataPln = NULL;
    FLASH_DATA_PAGE *pDataPage = NULL;

    FLASH_TALBE_PAGE_MANT *pPageMnt = NULL;
    FLASH_TALBE_BLOCK_MANT *pBlockMnt = NULL;
    UINT32 nNextLogID = 0;
    FLASH_TALBE_PAGE_MANT *pCurPageMnt = NULL;

    
    // clear data page
    pDataPln = g_FlashCOMData[nPU].pDataPln[nPln];
    pDataPage = &(pDataPln->DataPage[nBlock][0]);
    memset(pDataPage, 0xFF, sizeof(FLASH_DATA_PAGE)*PG_PER_BLK);

   
    // clear table page
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
            pTablePage = &(pTablePln->PageData[nNextLogID]);

            memset(pTablePage, 0xFF, PG_SZ);
            nNextLogID = pCurPageMnt->NextLogID;
            
            pCurPageMnt->bValid = 0xFFFF;
            pCurPageMnt->NextLogID = INVALID_8F;
            pCurPageMnt->PhyPageID = INVALID_4F;
        }

        pBlockMnt->EndLogPge = INVALID_4F;
        pBlockMnt->StartLogPge = INVALID_4F;
    }
    
 
}

void Mid_Read_RedData(PFLASH_PHY pFlash_phy, char *pRedBuf)
{
    UINT32 nPageType = Flash_Talbe_GetPageType(pFlash_phy);

    if (RSV_DATA_TYPE == nPageType)
    {
        Flash_Table_Handle(pFlash_phy, NULL, pRedBuf, PG_SZ, FALSE);
    }
    else if (COM_DATA_TYPE == nPageType)
    {
        Flash_Data_Handle(pFlash_phy, NULL, pRedBuf, PG_SZ, FALSE);
    }

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


    for (uPuIndex = 0; uPuIndex < CE_SUM; uPuIndex++)
    {
        for (uPlnIndex = 0; uPlnIndex < 2; uPlnIndex++)
        {
            pDataPln = g_FlashCOMData[uPuIndex].pDataPln[uPlnIndex];
            for(ulBlockIndex = 0; ulBlockIndex < TOTAL_BLOCK_PER_PLN; ulBlockIndex++)
            {
                for(ulDataPageIndex = 0; ulDataPageIndex < PG_PER_BLK; ulDataPageIndex++)
                {
                    pDataPage = &(pDataPln->DataPage[ulBlockIndex][ulDataPageIndex]);
                    for(ulSectorIndex = 0; ulSectorIndex < SEC_PER_PG; ulSectorIndex++)
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
