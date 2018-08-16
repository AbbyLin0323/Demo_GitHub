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

#include "BaseDef.h"
#include "hfmid.h"
#include "hfmodule.h"
#include "sim_flash_config.h"
unsigned long TOBEY_PAGE_SIZE = 16 * 1024;
unsigned long PU_TOTAL_NUM = 4;

void SetFun(PMID_LEVEL_STR pMls, int nType)
{
    if(pMls->m_nType == LOCAL_PROCESS_MEMORY)
    {
        if(0 == nType)
        {
            pMls->Init = local_init;
            pMls->Read = local_read;
            pMls->Write = local_write;
            pMls->Erase = local_erase;
            pMls->UnInit = local_un_init;
        }
        else if(1 == nType)
        {
            pMls->Init = local_init_SATA;
            pMls->Read = local_read_SATA;
            pMls->Write = local_write_SATA;
            pMls->Erase = local_erase_SATA;
            pMls->UnInit = local_un_init_SATA;
        }

    }
    else if(pMls->m_nType == CROSS_PROCESS_MEMORY)
    {
        if(0 == nType)
        {
            pMls->Init = crs_init;
            pMls->Read = crs_read;
            pMls->Write = crs_write;
            pMls->Erase = crs_erase;
            pMls->UnInit = crs_un_init;
        }
        else if(1 == nType)
        {
            pMls->Init = crs_init_SATA;
            pMls->Read = crs_read_SATA;
            pMls->Write = crs_write_SATA;
            pMls->Erase = crs_erase_SATA;
            pMls->UnInit = crs_un_init_SATA;
        }
    }
    else
    {
        printf("set Fun should not be here\n");
        //DBG_Getch();DBG_Getch();
    }
}

void Mid_Init(int nPu, int nPln, int nBlk, int nPge, unsigned long ulCapacity, unsigned long ulPgeSize, int nRsvPer)
{
    //Dyn decide which group interface to be used
    unsigned long ulRsvSize = 0;
    HANDLE hFile = 0;
    unsigned long ulPgeInitValue = ~MEMFILE_MASK;
    int nPgeArrayIndex = 0;
    int nHMPge, nHMRsv;
    __int64 i64Cap = 0;
    DWORD dwRet = 0;

    int nIndex = 0;
    int nInterface;
    HANDLE  hFile1;
    gmls.m_fp.gnCapacity = ulCapacity;
    nInterface = Dec_Interface(ulCapacity);
    gmls.m_nType = nInterface;
    gmls.m_fp.gnHowManyBlk = nBlk;
    gmls.m_fp.gnHowManyPge = nPge;
    gmls.m_fp.gnHowManyPln = nPln;
    gmls.m_fp.gnHowManyPU = nPu;
    gmls.m_fp.gnPageType = ulPgeSize;
    TOBEY_PAGE_SIZE = ulPgeSize;

    SetFun(&gmls, 0);
    ulRsvSize = gmls.Init(&gmls.m_fp, nRsvPer);

    //Now we will allocate the local info to manage the rsv buffer, in the unit of the ulPgeSize
    i64Cap = ulCapacity * 1024 * 1024 * 1024I64;
    nHMPge = i64Cap / ulPgeSize;


    hFile1 = CreateFile(SAVE_FILE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, // normal file
        NULL);
    if(INVALID_HANDLE_VALUE == hFile1)
    {
        gPgeInfo.nTotal = nHMPge;
        gPgeInfo.pPgeInfo = (unsigned long*)malloc(nHMPge * sizeof(unsigned long));
        if (gPgeInfo.pPgeInfo == NULL)
        {
            printf("Mid_Init  gPgeInfo.pPgeInfo == NULL\n");
        }
        for(;nPgeArrayIndex < gPgeInfo.nTotal; nPgeArrayIndex++)
            memcpy(gPgeInfo.pPgeInfo + nPgeArrayIndex, &ulPgeInitValue, 4);

        gBMR.nTotal = ulRsvSize / gmls.m_fp.gnPageType;
        gBMR.nHead = gBMR.nTail = 0;
        gBMR.ulCurNum = 0;
        gBMR.pBuf = (unsigned long*)malloc(gBMR.nTotal * sizeof(unsigned long));
        if (gBMR.pBuf == NULL)
        {
            printf("Mid_Init gBMR.pBuf == NULL \n");
        }
        for(;nIndex < gBMR.nTotal; nIndex++)
            gBMR.pBuf[nIndex] = nIndex;

        Init_FMR(RSV_FILE_SIZE, TOBEY_PAGE_SIZE);
    }
    else
    {
        ReadFile(hFile1, &gPgeInfo, sizeof(PGE_INFO), &dwRet, NULL);
        gPgeInfo.pPgeInfo = (unsigned long*)malloc(gPgeInfo.nTotal * sizeof(unsigned long));
        ReadFile(hFile1, gPgeInfo.pPgeInfo, gPgeInfo.nTotal * sizeof(unsigned long), &dwRet, NULL);
        ReadFile(hFile1, &gBMR, sizeof(BUF_MANAGE_RSV), &dwRet, NULL);
        gBMR.pBuf = (unsigned long*)malloc(gBMR.nTotal * sizeof(unsigned long));
        ReadFile(hFile1, gBMR.pBuf, sizeof(unsigned long) * gBMR.nTotal, &dwRet, NULL);
        ReadFile(hFile1, &gFMR, sizeof(FILE_MANAGE_RSV), &dwRet, NULL);
        gFMR.pRsvIndex = (unsigned long*)malloc(gFMR.nHWRsvIndex * sizeof(unsigned long));
        ReadFile(hFile1, gFMR.pRsvIndex, gFMR.nHWRsvIndex * sizeof(unsigned long), &dwRet, NULL);
    }
}

__int64 FPos2MPos(PFLASH_PHY pFlash_phy, int nType)
{
    if(nType == COM_DATA_TYPE)
        return FPos2MPos_Com(pFlash_phy);
    else if(nType == RAND_DATA_TYPE)
        return FPos2MPos_Rand(pFlash_phy);
    else if(nType == RSV_DATA_TYPE)
        return FPos2MPos_Rsv(pFlash_phy);
    return -1;
}

__int64 FPos2MPos_Com(PFLASH_PHY pFlash_phy)
{
    __int64 ulOffset = 0;
    int nPU = pFlash_phy->nPU;
    int nPln = pFlash_phy->nPln;
    int nBlk = pFlash_phy->nBlock;
    int nPge = pFlash_phy->nPage;

    ulOffset = ONE_PU_FILE_SIZE(gmls.m_fp) * nPU + ONE_PLN_FILE_SIZE(gmls.m_fp) * nPln + ONE_BLK_FILE_SIZE(gmls.m_fp) * nBlk + nPge;
    ulOffset *= (COM_DATA_SIZE * gmls.m_fp.gnPageType / 512);
    return ulOffset;

    return 1;
}
__int64 FPos2MPos_Rand(PFLASH_PHY pFlash_phy)
{
    unsigned long ulOffset = 0;
    int nPU = pFlash_phy->nPU;
    int nPln = pFlash_phy->nPln;
    int nBlk = pFlash_phy->nBlock;
    int nPge = pFlash_phy->nPage;

    ulOffset = ONE_PU_FILE_SIZE(gmls.m_fp) * nPU + ONE_PLN_FILE_SIZE(gmls.m_fp) * nPln + ONE_BLK_FILE_SIZE(gmls.m_fp) * nBlk + nPge;
    ulOffset *= RAND_DATA_SIZE;
    return ulOffset;
    return 1;
}

__int64 FPos2MPos_Rsv(PFLASH_PHY pFlash_phy)
{
    int nPU = pFlash_phy->nPU;
    int nPln = pFlash_phy->nPln;
    int nBlk = pFlash_phy->nBlock;
    int nPge = pFlash_phy->nPage;

    unsigned long ulArrayIndex = ONE_PU_FILE_SIZE(gmls.m_fp) * nPU + ONE_PLN_FILE_SIZE(gmls.m_fp) * nPln + ONE_BLK_FILE_SIZE(gmls.m_fp) * nBlk + nPge;
    switch(gPgeInfo.pPgeInfo[ulArrayIndex] & MEMFILE_MASK)
    {
    case IN_MEMORY:
        return gPgeInfo.pPgeInfo[ulArrayIndex];
        break;
    case IN_FILE:
        return gPgeInfo.pPgeInfo[ulArrayIndex];
        break;
    default:
        return NOT_FOUND;
        break;
    }
    return 1;
}

static int Dec_Interface(unsigned long ulCapacity)
{
    int nRet = 0;
    if(ulCapacity <= 32)
        nRet = LOCAL_PROCESS_MEMORY;
    else
        nRet = CROSS_PROCESS_MEMORY;

    if(LOCAL_PROCESS_MEMORY == nRet)
        COM_DATA_SIZE = 5;
    else if(CROSS_PROCESS_MEMORY == nRet)
        COM_DATA_SIZE = 8;
    return nRet;
}

void Mid_Read(PFLASH_PHY pFlash_phy, int nType, char* pBuf, int nLength)
{
    MID_PARA_STR mps;
    __int64 i64FP;
    char* pTmp;
    int nNum = 0;
    DWORD dwFF = 0xFFFFFFFF;
    int nIndex = 0;
    __int64 ulAddr = FPos2MPos(pFlash_phy, nType);
    Check(22, 0, 157, 242, pFlash_phy);
    memset(pBuf, 0, nLength);
    //printf("read %d, %d, %d, %d, %d\n", nType, pFlash_phy->nPU, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
    if(0 == (ulAddr & MEMFILE_MASK) && (nType == RSV_DATA_TYPE)
        || (ulAddr == -1) && (nType != RSV_DATA_TYPE))
    {
        memset(pBuf, 0xFF, nLength);
        //printf("error get the 0xff value\n");
        //DBG_Getch();DBG_Getch();
        return;
    }
    else
    {
        mps.dwLen = nLength;
        mps.nNum = pFlash_phy->nPU;
        mps.nType = nType;
        mps.pvalue = pBuf;
        mps.dwStartAddrHigh = (ulAddr >> 32 ) & 0xFFFFFFFF;
        mps.dwStartAddr = ulAddr & 0xFFFFFFFF;
        //Just rsv data can be saved in the file.
        if((nType == COM_DATA_TYPE) || (nType == RAND_DATA_TYPE))
        {
            mps.nInMem = IN_MEMORY;
            if(nType == COM_DATA_TYPE)
            {
                nNum = nLength / 512;
                pTmp = (char*)malloc(nNum * COM_DATA_SIZE);

                if (pTmp == NULL)
                {
                    printf("Mid_Read pTmp == NULL \n");
                }

                memset(pTmp, 0, nNum * COM_DATA_SIZE);

                mps.pvalue = pTmp;
                mps.dwLen = nNum * COM_DATA_SIZE;
            }
        }
        else if(nType == RSV_DATA_TYPE)
        {
            mps.nInMem = ulAddr & MEMFILE_MASK;
            i64FP = (__int64)(((__int64)ulAddr) & (~MEMFILE_MASK)) * TOBEY_PAGE_SIZE;
            mps.dwStartAddr = i64FP & 0xFFFFFFFF;
            mps.dwStartAddrHigh = (i64FP >> 32) & 0xFFFFFFFF;
        }
        gmls.Read(&mps);
        if(nType == COM_DATA_TYPE)
        {
            for(nIndex = 0; nIndex < nNum; nIndex++)
                memcpy(pBuf + 512 * nIndex, pTmp + nIndex * COM_DATA_SIZE, COM_DATA_SIZE);
            free(pTmp);
        }
    }
    if(memcmp(pBuf, &dwFF, 4) == 0)
    {
        //printf("read ff %d, %d, %d, %d\n", pFlash_phy->nPU, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);
        //DBG_Getch();DBG_Getch();
    }

}
void Mid_Write(PFLASH_PHY pFlash_phy, int nType, char* pBuf, int nLength)
{
    MID_PARA_STR mps;
    static int nCount = 0;
    __int64 i64FP;
    int nCBAddr = 0;
    char* pTmp;
    int nNum = 0;
    int nIndex = 0;
    unsigned long ulIndex = 0;
    unsigned long ulArrayIndex = 0;
    int nPU = pFlash_phy->nPU;
    int nPln = pFlash_phy->nPln;
    int nBlk = pFlash_phy->nBlock;
    int nPge = pFlash_phy->nPage;
    __int64 ulAddr = FPos2MPos(pFlash_phy, nType);
    mps.dwLen = nLength;
    mps.nNum = pFlash_phy->nPU;
    mps.nType = nType;
    mps.pvalue = pBuf;

    //printf("write %d, %d, %d, %d, %d\n", nType, pFlash_phy->nPU, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);

    Check(22, 0, 157, 242, pFlash_phy);
    if((nType == COM_DATA_TYPE) || (nType == RAND_DATA_TYPE))
    {
        mps.nInMem = IN_MEMORY;
        mps.dwStartAddrHigh = (ulAddr >> 32) & 0xFFFFFFFF;
        mps.dwStartAddr = ulAddr & 0xFFFFFFFF;
        if(nType == COM_DATA_TYPE)
        {
            nNum = nLength / 512;
            pTmp = (char*)malloc(nNum * COM_DATA_SIZE);

            if (pTmp == NULL)
            {
                printf("Mid_Write pTmp == NULL \n");
            }

            for(nIndex = 0; nIndex < nNum; nIndex++)
                memcpy(pTmp + nIndex * COM_DATA_SIZE, pBuf + 512 * nIndex, COM_DATA_SIZE);
            mps.pvalue = pTmp;
            mps.dwLen = nNum * COM_DATA_SIZE;
        }
        gmls.Write(&mps);
        if(nType == COM_DATA_TYPE)free(pTmp);
    }
    else if(nType == RSV_DATA_TYPE)
    {
        if(0 == (ulAddr & MEMFILE_MASK))//Represent that the page has not been write
        {
            ulIndex = GetOne(&gBMR);
            if(NO_INDEX == ulIndex)
            {
                ulArrayIndex = ONE_PU_FILE_SIZE(gmls.m_fp) * nPU + ONE_PLN_FILE_SIZE(gmls.m_fp) * nPln + ONE_BLK_FILE_SIZE(gmls.m_fp) * nBlk + nPge;
                if(gPgeInfo.pPgeInfo[ulArrayIndex] != ~MEMFILE_MASK)
                {
                    printf("over write the same page\n, it is not a right behavior\n");
                    DBG_Getch();
                }
                gPgeInfo.pPgeInfo[ulArrayIndex] = Get_FMR();

                i64FP = (__int64)(((__int64)gPgeInfo.pPgeInfo[ulArrayIndex]) & (~MEMFILE_MASK)) * TOBEY_PAGE_SIZE;//This place can get the file postition, according to different stratgy
                mps.dwStartAddr = i64FP & 0xFFFFFFFF;
                mps.dwStartAddrHigh = (i64FP >> 32) & 0xFFFFFFFF;
                gPgeInfo.pPgeInfo[ulArrayIndex] |= IN_FILE;
                mps.nInMem = IN_FILE;
                gmls.Write(&mps);
            }
            else
            {
                i64FP = (__int64)(((__int64)ulIndex)) * TOBEY_PAGE_SIZE;//This place can get the file postition, according to different stratgy

                mps.dwStartAddr = i64FP & 0xFFFFFFFF;
                mps.dwStartAddrHigh = (i64FP >> 32) & 0xFFFFFFFF;//need high.???
                mps.nInMem = IN_MEMORY;
                gmls.Write(&mps);
                ulArrayIndex = ONE_PU_FILE_SIZE(gmls.m_fp) * nPU + ONE_PLN_FILE_SIZE(gmls.m_fp) * nPln + ONE_BLK_FILE_SIZE(gmls.m_fp) * nBlk + nPge;
                gPgeInfo.pPgeInfo[ulArrayIndex] = ulIndex | IN_MEMORY;

            }
        }
        else
        {
            i64FP = (__int64)(((__int64)ulAddr) & (~MEMFILE_MASK)) * TOBEY_PAGE_SIZE;
            mps.dwStartAddr = i64FP & 0xFFFFFFFF;
            mps.dwStartAddrHigh = (i64FP>>32)&0xFFFFFFFF;
            mps.nInMem = ulAddr & MEMFILE_MASK;

            nCount++;
            gmls.Write(&mps);
        }
    }

}
void Mid_Erase(PFLASH_PHY pFlash_phy)
{
    unsigned long ulLen;
    MID_PARA_STR mps;

    int nPgeIndex;
    __int64 ulAddr;
    unsigned long ulArrayIndex;
    int nPU = pFlash_phy->nPU;
    int nPln = pFlash_phy->nPln;
    int nBlk = pFlash_phy->nBlock;
    char* pBuf = NULL;
    pFlash_phy->nPage = 0;
    ulLen = COM_DATA_SIZE * TOBEY_PAGE_SIZE / 512 *gmls.m_fp.gnHowManyPge;
    ulAddr = FPos2MPos(pFlash_phy, COM_DATA_TYPE);
    mps.dwLen = ulLen;
    mps.nNum = pFlash_phy->nPU;
    mps.nType = COM_DATA_TYPE;
    pBuf = (char*)malloc(ulLen);
    if (pBuf == NULL)
    {
        printf("Mid_Erase: pBuf == NULL");
    }
    memset(pBuf, 0xFF, ulLen);
    mps.pvalue = pBuf;
    mps.dwStartAddr = ulAddr & 0xFFFFFFFF;
    mps.dwStartAddrHigh = (ulAddr >> 32) & 0xFFFFFFFF;
    mps.nInMem = IN_MEMORY;

    Check(22, 0, 157, 242, pFlash_phy);
    //printf("erase %d, %d, %d, %d\n", pFlash_phy->nPU, pFlash_phy->nPln, pFlash_phy->nBlock, pFlash_phy->nPage);


    gmls.Write(&mps);
    free(pBuf);

    ulLen = RAND_DATA_SIZE * gmls.m_fp.gnHowManyPge;
    pBuf = (char*)malloc(ulLen);
    memset(pBuf, 0xFF, ulLen);
    Mid_Write(pFlash_phy, RAND_DATA_TYPE, pBuf, ulLen);
    free(pBuf);

    for(nPgeIndex = 0; nPgeIndex < gmls.m_fp.gnHowManyPge; nPgeIndex++)
    {
        ulArrayIndex = ONE_PU_FILE_SIZE(gmls.m_fp) * nPU + ONE_PLN_FILE_SIZE(gmls.m_fp) * nPln + ONE_BLK_FILE_SIZE(gmls.m_fp) * nBlk + nPgeIndex;
        ulAddr = gPgeInfo.pPgeInfo[ulArrayIndex];
        if((ulAddr & MEMFILE_MASK) == IN_MEMORY)
        {
            gPgeInfo.pPgeInfo[ulArrayIndex] = ~MEMFILE_MASK;
            RsyOne(&gBMR, ulAddr & (~MEMFILE_MASK));
        }
        else if((ulAddr & MEMFILE_MASK) == IN_FILE)
            //gPgeInfo.pPgeInfo[ulArrayIndex] &= ~MEMFILE_MASK;
        {
            gPgeInfo.pPgeInfo[ulArrayIndex] = ~MEMFILE_MASK;
            Set_FMR(ulAddr & (~MEMFILE_MASK));
        }
        else
        {
        }
    }
}

void Mid_Un_Init()
{
    DWORD dwRet = 0;
    HANDLE  hFile = CreateFile(SAVE_FILE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, // normal file
        NULL);
    if(INVALID_HANDLE_VALUE == hFile)
    {
        printf("SAVE_FILE_NAME file failed\n");
        //DBG_Getch();DBG_Getch();
    }

    printf("you have closing the ssd firmware, remember all the information\n");

    //Remember all the information
    WriteFile(hFile, &gPgeInfo, sizeof(PGE_INFO), &dwRet, NULL);
    WriteFile(hFile, gPgeInfo.pPgeInfo, sizeof(unsigned long) * gPgeInfo.nTotal, &dwRet, NULL);

    WriteFile(hFile, &gBMR, sizeof(BUF_MANAGE_RSV), &dwRet, NULL);
    WriteFile(hFile, gBMR.pBuf, gBMR.nTotal * sizeof(unsigned long), &dwRet, NULL);

    WriteFile(hFile, &gFMR, sizeof(FILE_MANAGE_RSV), &dwRet, NULL);
    WriteFile(hFile, gFMR.pRsvIndex, sizeof(unsigned long) * gFMR.nHWRsvIndex, &dwRet, NULL);

    gmls.UnInit();

    MID_Un_Init_SATA();
}

//Sata Interface
void Mid_Init_SATA(unsigned long ulCapacity)
{
    int nRet = Dec_Interface(ulCapacity);
    gmlss.m_nType = nRet;
    SetFun(&gmlss, 1);
    gmlss.Init(ulCapacity);
}
void Mid_Read_SATA(unsigned long ulLBA, char* pBuf, int nLength){
    MID_PARA_STR mps;
    __int64 ulLBARealAddr = (__int64)((__int64)ulLBA * 4);
    memset(pBuf, 0, nLength);
    mps.dwLen = nLength;
    mps.dwStartAddr = ulLBARealAddr & 0xFFFFFFFF;
    mps.dwStartAddrHigh = (ulLBARealAddr >> 32) & 0xFFFFFFFF;
    mps.nInMem = IN_MEMORY;
    mps.nType = COM_DATA_TYPE;
    mps.pvalue = pBuf;
    gmlss.Read(&mps);
}
void Mid_Write_SATA(unsigned long ulLBA, char* pBuf, int nLength){
    MID_PARA_STR mps;
    __int64 ulLBARealAddr = (__int64)((__int64)ulLBA * 4);
    mps.dwLen = nLength;
    mps.dwStartAddr = ulLBARealAddr & 0xFFFFFFFF;
    mps.dwStartAddrHigh = (ulLBARealAddr >> 32) & 0xFFFFFFFF;
    mps.nInMem = IN_MEMORY;
    mps.nType = COM_DATA_TYPE;
    mps.pvalue = pBuf;
    gmlss.Write(&mps);
}
void Mid_Erase_SATA(unsigned long ulLBA){
    int nLength = 4;
    char pBuf = (char*)malloc(nLength);
    memset(pBuf, 0xFF, nLength);
    Mid_Write_SATA(ulLBA, pBuf, nLength);
    free(pBuf);
}
void MID_Un_Init_SATA(){
    gmlss.UnInit();
}

int Check(int nPU, int nPln, int nBlk, int nPge, PFLASH_PHY pFP)
{
    if ((pFP->nPU == nPU)
        && (pFP->nPln == nPln)
        && (pFP->nBlock == nBlk)
        && (pFP->nPage == nPge)
        )

    {
        //DBG_Getch() ;
    }
    else
        return 1;
}