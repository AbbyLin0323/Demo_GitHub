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


#include "hfmodule.h"
#include "sim_flash_config.h"
FILE_BASE_POS gfbp;
unsigned long gulSingleSize_com;
unsigned long gulSingleSize_rand;
unsigned long gulPSTotal;
HANDLE ghRsvFile_crs;

void SetMPS(PMID_PARA_STR pmps, int nNum,unsigned long  dwStartAddr,unsigned long  dwLen,int nType, char* pvalue, int nInMem)
{
    pmps->nNum = nNum;
    pmps->dwLen = dwLen;
    pmps->dwStartAddr = dwStartAddr;
    pmps->nType = nType;
    pmps->pvalue = pvalue;
    pmps->nInMem = nInMem;
}

int crs_init(PFLASH_PROPERTY pFPro, int nperRsv)
{
    int nPSTotal = 0, nPSRand = 0, nPSCom = 0, nPSRsv = 0;
    int nPSIndex = 0;
    char PSName[64];
    DWORD dwError = 0;
    nPSCom = Com_Init(pFPro->gnCapacity, 0);
    nPSRand = Rand_Init(pFPro->gnCapacity);
    nPSRsv = Rsv_Init(pFPro->gnCapacity, nperRsv);
    ZeroMemory(PSName, 64);
    SetLastError(0);

    nPSTotal = nPSRand + nPSCom + nPSRsv;
    gulPSTotal = nPSTotal;
    gpCPI = (PCRS_PS_INFO)malloc(sizeof(CRS_PS_INFO) * nPSTotal);
    if (gpCPI == NULL)
    {

        printf("crs_init gpCPI == NULL \n");

    }
    ZeroMemory(gpCPI, 0, sizeof(CRS_PS_INFO) * nPSTotal);
    for(nPSIndex = 0; nPSIndex < nPSCom; nPSIndex++)
    {
        sprintf(PSName, COM_MAPNAME, nPSIndex);
        gpCPI[nPSIndex].nType = MAP_TYPE_COM;
        gpCPI[nPSIndex].bMapped = 0;
        do{
            gpCPI[nPSIndex].hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PSName);
            dwError = GetLastError();
        }while(gpCPI[nPSIndex].hFileMapping == NULL);

    }
    for(;nPSIndex < nPSRand + nPSCom; nPSIndex++)
    {
        sprintf(PSName, RAND_MAPNAME, nPSIndex - nPSCom);
        gpCPI[nPSIndex].nType = MAP_TYPE_RAND;
        gpCPI[nPSIndex].bMapped = 0;
        do{
            gpCPI[nPSIndex].hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PSName);
            dwError = GetLastError();

        }while(gpCPI[nPSIndex].hFileMapping == NULL || (dwError != 0));
        if(NULL == gpCPI[nPSIndex].hFileMapping)
        {
            {
                printf("error code %d", GetLastError());
            }
        }
    }
    for(;nPSIndex < nPSRand + nPSCom + nPSRsv; nPSIndex++)
    {
        sprintf(PSName, RSV_MAPNAME, nPSIndex - nPSCom - nPSRand);
        gpCPI[nPSIndex].nType = MAP_TYPE_RSV;
        gpCPI[nPSIndex].bMapped = 0;
        do{
            gpCPI[nPSIndex].hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PSName);
            dwError = GetLastError();

        }while(gpCPI[nPSIndex].hFileMapping == NULL);
    }

    ghRsvFile_crs = CreateFile(RSV_FILE_NAME_CRS,               // file to open
        GENERIC_READ | GENERIC_WRITE,          // open for reading
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
        NULL,                  // default security
        CREATE_ALWAYS,         // existing file only
        FILE_ATTRIBUTE_NORMAL, // normal file
        NULL);
    if(INVALID_HANDLE_VALUE == gfbp.hRsvFile)
    {
        printf("rsv file failed\n");
        //DBG_Getch();DBG_Getch();
    }
    return nPSRsv * 1024 * 1024 * 1024;
}

int crs_read(PMID_PARA_STR pmps)
{
    ExeOpe(pmps, 0x02, 0);
    return 1;
}
int crs_write(PMID_PARA_STR pmps)
{
    ExeOpe(pmps, 0x01, 0);
    return 1;
}

int crs_erase(PMID_PARA_STR pmps)
{
    //char* pFF = (char*)malloc(pmps->dwLen);
    //memset(pFF, 0xFF, pmps->dwLen);
    ExeOpe(pmps, 0x01, 0);
    //free(pFF);
}

int Rand_Init(unsigned long dwCapacity)
{
    //Ran
    __int64 ulSize;
    char PSName[64];
    char PSArg[64];
    int nPSIndex = 0;
    ZeroMemory(PSName, 64);
    ZeroMemory(PSArg, 64);
    ulSize = dwCapacity * 1024 * 1024 * 1024I64;
    ulSize = ulSize / TOBEY_PAGE_SIZE * RAND_DATA_SIZE;
    gulSingleSize_rand = ulSize;
    nPSIndex = 0;

    sprintf(PSName, RAND_MAPNAME, nPSIndex);
    sprintf(PSArg, ARG_LIST, PSName, ulSize);

    //if(0 == CreatePS(PSArg))
    //    DBG_Getch();DBG_Getch();
    return 1;
}

int Com_Init(unsigned long dwCapacity, int nType)//Need to modify the process Number according to the capacity
{
    char PSName[64];
    char PSArg[64];
    int nPSIndex = 0;

    __int64 ulSize = dwCapacity * 1024 * 1024 * 1024I64 / PS_NUMBER_COM;
    unsigned long ulSglPSi = 0;

    ulSglPSi = TOBEY_PAGE_SIZE / 512 * COM_DATA_SIZE;
    ulSize = ulSize / TOBEY_PAGE_SIZE * ulSglPSi;

    gulSingleSize_com = ulSize;

    ZeroMemory(PSName, 64);
    ZeroMemory(PSArg, 64);

    for(nPSIndex = 0; nPSIndex < PS_NUMBER_COM; nPSIndex++)
    {
        if(nType == 0)
            sprintf(PSName, COM_MAPNAME, nPSIndex);
        else if(1== nType)
            sprintf(PSName, COM_MAPNAME_SATA, nPSIndex);
        sprintf(PSArg, ARG_LIST, PSName, ulSize);

        //if(0 == CreatePS(PSArg))
        //    DBG_Getch();DBG_Getch();
    }
    return PS_NUMBER_COM;
}

int Rsv_Init(unsigned long dwCapacity, int nPerRsv)
{
    char PSName[64];
    char PSArg[64];
    int nHMInx;
    int nHMPro = 0;
    double dbSize = 0;
    dbSize = (double)dwCapacity * nPerRsv / 1000;
    nHMPro = (int)dbSize;
    if((double)nHMPro < dbSize)
        nHMPro++;
    for(nHMInx = 0; nHMInx < nHMPro; nHMInx++)
    {
        sprintf(PSName, RSV_MAPNAME, nHMInx);
        sprintf(PSArg, ARG_LIST, PSName, ONE_RSV_SIZE);
        //if(0 == CreatePS(PSArg))
        //    DBG_Getch();DBG_Getch();
    }

    return nHMPro;

    //Construct RSV file

}

int CreatePS(char* pSArg)
{
    HANDLE hEve = NULL;
    SECURITY_ATTRIBUTES attr;
    PSECURITY_DESCRIPTOR pSec;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char Buf[MAX_PATH];;

    ZeroMemory(Buf, MAX_PATH);
    GetCurrentDirectory(MAX_PATH, Buf);
    strcat(Buf, EXE_FILE_NAME);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
    hEve = CreateEvent(NULL, TRUE, FALSE, "Global\\Eve");
    if(hEve == NULL)
    {
        DWORD dwError = GetLastError();
        printf("Create Event failed, Error code = %d\n", dwError);
    }

    pSec = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if(!pSec)
    {
        return GetLastError();
    }
    if(!InitializeSecurityDescriptor(pSec, SECURITY_DESCRIPTOR_REVISION))
    {
        LocalFree(pSec);
        return GetLastError();
    }
    if(!SetSecurityDescriptorDacl(pSec, TRUE, NULL, TRUE))
    {
        LocalFree(pSec);
        return GetLastError();
    }

    attr.bInheritHandle = FALSE;
    attr.lpSecurityDescriptor = pSec;
    attr.nLength = sizeof(SECURITY_ATTRIBUTES);

    if( !CreateProcess( Buf,
        pSArg,
        &attr,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE  ,
        NULL,
        NULL,
        &si,
        &pi )
        )
    {
        DWORD dwError = GetLastError();
        printf("Create Process failed, Error code = %d\n", dwError);
        //DBG_Getch();DBG_Getch();
        return 0;
    }

    LocalFree(pSec);
    WaitForSingleObject(hEve, INFINITE);
    CloseHandle(hEve);
    return 1;
}

int PU2CrsPSNum(__int64* i64StartAddr, int nType)
{
    int nArrayIndex = 0;
    switch(nType)
    {
    case MAP_TYPE_COM:
        nArrayIndex = *i64StartAddr / gulSingleSize_com;
        *i64StartAddr %= gulSingleSize_com;
        break;
    case MAP_TYPE_RAND:
        nArrayIndex = 4;
        break;
    case MAP_TYPE_RSV:
        nArrayIndex = *i64StartAddr / ONE_RSV_SIZE + 5;
        *i64StartAddr %= ONE_RSV_SIZE;
        break;
    }
    return nArrayIndex;
}

int ExeOpe(PMID_PARA_STR pmps, int nDirect, int nSorF)
{
    int nNeedReMap = 0;
    static int nExeOpeCount = 0;
    DWORD dwMapStart = 0;
    PCRS_PS_INFO pcpi = NULL;
    int nArrayIndex = 0 ;
    DWORD ulOut = 0;
    DWORD dwError = 0;
    unsigned long ulFLow = 0, ulFHigh = 0;
    __int64 i64FilePointer = 0;
    int nPuNum = pmps->nNum;
    unsigned long  dwStartAddr = pmps->dwStartAddr;
    unsigned long dwStartAddrHigh = pmps->dwStartAddrHigh;
    __int64 i64Pos = 0;
    unsigned long  dwLen = pmps->dwLen;
    char* pvalue = pmps->pvalue;
    int nType = pmps->nType;
    unsigned long ulMapSize = 0;
    if(nSorF == 0)
        pcpi = gpCPI;
    else if(1 == nSorF)
        pcpi = gpCPI_Sata;
    else
    {
        printf("code should not be here\n");
        //DBG_Getch();DBG_Getch();
    }

    SetLastError(0);
    nExeOpeCount++;
    i64Pos = (__int64)((((__int64)dwStartAddrHigh) << 32) | dwStartAddr);
    dwStartAddr = i64Pos & 0xFFFFFFFF;
    dwStartAddrHigh = (i64Pos >> 32) & 0xFFFFFFFF;
    if(pmps->nInMem == IN_MEMORY)
    {
        nArrayIndex = PU2CrsPSNum(&i64Pos, nType);
        dwStartAddr = i64Pos & 0xFFFFFFFF;
        dwStartAddrHigh = (i64Pos >> 32) & 0xFFFFFFFF;
        if(pcpi[nArrayIndex].bMapped == 1)
        {
            if(pcpi[nArrayIndex].ulMappedLen < dwLen)
            {
                printf("no way\n");
                //DBG_Getch();
            }
            if((dwStartAddr >= pcpi[nArrayIndex].ulStartPos) &&
                (dwStartAddr <= pcpi[nArrayIndex].ulStartPos + pcpi[nArrayIndex].ulMappedLen - dwLen))
            {
                if(nDirect == 0x01)
                    MoveMemory(pcpi[nArrayIndex].pMapped + dwStartAddr - pcpi[nArrayIndex].ulStartPos, pvalue, dwLen);
                else
                    MoveMemory(pvalue, pcpi[nArrayIndex].pMapped + dwStartAddr - pcpi[nArrayIndex].ulStartPos, dwLen);
                return 1;
            }
            else
            {
                UnmapViewOfFile(pcpi[nArrayIndex].pMapped);
                nNeedReMap = 1;
            }
        }
        if((pcpi[nArrayIndex].bMapped == 0) ||
            (nNeedReMap == 1))
        {
            dwMapStart = dwStartAddr / (64 * 1024) * 64 * 1024;
            switch(nType)
            {
            case MAP_TYPE_COM:
                if(dwMapStart + 2048 * TOBEY_PAGE_SIZE <= gulSingleSize_com)
                    ulMapSize = 2048 * TOBEY_PAGE_SIZE;
                else
                    ulMapSize = gulSingleSize_com - dwMapStart;
                break;
            case MAP_TYPE_RAND:
                if(dwMapStart + 2048 * TOBEY_PAGE_SIZE <= gulSingleSize_rand)
                    ulMapSize = 2048 * TOBEY_PAGE_SIZE;
                else
                    ulMapSize = gulSingleSize_rand - dwMapStart;
                break;
            case MAP_TYPE_RSV:
                if(dwMapStart + 2048 * TOBEY_PAGE_SIZE <= ONE_RSV_SIZE)
                    ulMapSize = 2048 * TOBEY_PAGE_SIZE;
                else
                    ulMapSize = (unsigned long)ONE_RSV_SIZE - dwMapStart;
                break;
            default:
                {
                    printf("should not be here\n");
                    //DBG_Getch();DBG_Getch();
                }
            }




            /*do{
            pcpi[nArrayIndex].pMapped = (char*)MapViewOfFile(
            pcpi[nArrayIndex].hFileMapping,
            FILE_MAP_ALL_ACCESS,
            dwStartAddrHigh,
            dwMapStart,
            ulMapSize
            );;
            dwError = GetLastError();
            }while(dwError == 0);*/
            pcpi[nArrayIndex].pMapped = (char*)MapViewOfFile(
                pcpi[nArrayIndex].hFileMapping,
                FILE_MAP_ALL_ACCESS,
                dwStartAddrHigh,
                dwMapStart,
                ulMapSize
                );;

            dwError = GetLastError();
            while (dwError != 0)
            {
                printf("map view error\n");
            }
            if(pcpi[nArrayIndex].pMapped == NULL)
            {
                {
                    DWORD dwError = GetLastError();
                    printf("why %d", dwError);
                    //DBG_Getch();DBG_Getch();
                    //DBG_Getch();
                }
            }

            if(nDirect == 0x01)
                MoveMemory(pcpi[nArrayIndex].pMapped + dwStartAddr - dwMapStart, pvalue, dwLen);
            else
                MoveMemory(pvalue, pcpi[nArrayIndex].pMapped  + dwStartAddr - dwMapStart, dwLen);
            pcpi[nArrayIndex].ulStartPos = dwMapStart;
            pcpi[nArrayIndex].ulMappedLen = ulMapSize;
            pcpi[nArrayIndex].bMapped = 1;
        }
    }
    else if(pmps->nInMem == IN_FILE)
    {
        if(nDirect == 0x01)
        {
            if(pmps->dwStartAddr == (~MEMFILE_MASK))
            {
                ulFLow = GetFileSize(ghRsvFile_crs, &ulFHigh);
                i64FilePointer = (__int64)((((__int64)ulFHigh) << 32I64) | ulFLow);
                pmps->dwStartAddr = i64FilePointer / TOBEY_PAGE_SIZE;
                SetFilePointer(ghRsvFile_crs, ulFLow, &ulFHigh, FILE_BEGIN);
                WriteFile(ghRsvFile_crs, pmps->pvalue, pmps->dwLen, &ulOut, NULL);
            }
            else
            {
                SetFilePointer(ghRsvFile_crs, pmps->dwStartAddr, &(pmps->dwStartAddrHigh), FILE_BEGIN);
                WriteFile(ghRsvFile_crs, pmps->pvalue, pmps->dwLen, &ulOut, NULL);
            }
        }
        else
        {
            SetFilePointer(ghRsvFile_crs, pmps->dwStartAddr, &(pmps->dwStartAddrHigh), FILE_BEGIN);
            ReadFile(ghRsvFile_crs, pmps->pvalue, pmps->dwLen, &ulOut, NULL);
        }
    }
}


int crs_un_init()
{
    int nPSIndex = 0;
    for(;nPSIndex < gulPSTotal; nPSIndex++)
    {
        if(gpCPI[nPSIndex].bMapped == 1)
            UnmapViewOfFile(gpCPI[nPSIndex].pMapped);
        CloseHandle(gpCPI[nPSIndex].hFileMapping);
    }
    return 1;
}

//local operation
int local_init(PFLASH_PROPERTY pFPro, int nperRsv)//Make sure the capacity is lower than the 32G
{
    unsigned long ulCom, ulRand, ulTotalCRand;
    ulCom = Com_Init_Local(pFPro->gnCapacity);
    ulRand = Rand_Init_Local(pFPro->gnCapacity);
    ulTotalCRand = ulCom + ulRand;
    return Rsv_Init_Local(ulTotalCRand, 0);
}

int local_read(PMID_PARA_STR pmps)
{
    unsigned long ulOut = 0;
    DWORD dwFF = 0xFFFFFFFF;
    if(pmps->nType == MAP_TYPE_COM)
    {
        //pmps->dwLen = COM_DATA_SIZE;
        memcpy(pmps->pvalue, gfbp.pCom + pmps->dwStartAddr, pmps->dwLen);
    }
    else if(pmps->nType == MAP_TYPE_RAND)
    {
        memcpy(pmps->pvalue, gfbp.pRand + pmps->dwStartAddr, pmps->dwLen);
    }
    else if(pmps->nType == MAP_TYPE_RSV)
    {
        if(pmps->nInMem == IN_MEMORY)
            memcpy(pmps->pvalue, gfbp.pRsv + pmps->dwStartAddr, pmps->dwLen);
        else if(pmps->nInMem == IN_FILE)
        {
            //we shoulde  read it from the file
            //ReadFile(
            SetFilePointer(gfbp.hRsvFile, pmps->dwStartAddr, &(pmps->dwStartAddrHigh), FILE_BEGIN);
            ReadFile(gfbp.hRsvFile, pmps->pvalue, pmps->dwLen, &ulOut, NULL);
        }
        else
        {
        }
    }
    else
    {
        printf("local read should not be here\n");
        //DBG_Getch();DBG_Getch();
    }
    return 1;
}
int local_write(PMID_PARA_STR pmps)
{
    unsigned long ulFLow = 0, ulFHigh = 0, ulOut = 0;
    __int64 i64FilePointer;
    if(pmps->nType == MAP_TYPE_COM)
    {
        //pmps->dwLen = COM_DATA_SIZE;
        memcpy(gfbp.pCom + pmps->dwStartAddr, pmps->pvalue, pmps->dwLen);
    }
    else if(pmps->nType == MAP_TYPE_RAND)
    {
        memcpy(gfbp.pRand + pmps->dwStartAddr, pmps->pvalue, pmps->dwLen);
    }
    else if(pmps->nType == MAP_TYPE_RSV)
    {
        if(pmps->nInMem == IN_MEMORY)
            memcpy(gfbp.pRsv + pmps->dwStartAddr, pmps->pvalue, pmps->dwLen);
        else if(pmps->nInMem == IN_FILE)
        {
            //we shoulde  read it from the file
            //ReadFile(
            if(pmps->dwStartAddr == (~MEMFILE_MASK))
            {
                ulFLow = GetFileSize(gfbp.hRsvFile, &ulFHigh);
                i64FilePointer = (__int64)((((__int64)ulFHigh) << 32I64) | ulFLow);
                pmps->dwStartAddr = i64FilePointer / TOBEY_PAGE_SIZE;
                SetFilePointer(gfbp.hRsvFile, ulFLow, &ulFHigh, FILE_BEGIN);
                WriteFile(gfbp.hRsvFile, pmps->pvalue, pmps->dwLen, &ulOut, NULL);
            }
            else
            {
                SetFilePointer(gfbp.hRsvFile, pmps->dwStartAddr, &(pmps->dwStartAddrHigh), FILE_BEGIN);
                WriteFile(gfbp.hRsvFile, pmps->pvalue, pmps->dwLen, &ulOut, NULL);
            }
        }
    }
    else
    {
        printf("local read should not be here\n");
        //DBG_Getch();DBG_Getch();
    }
    return 1;
}

int local_erase(PMID_PARA_STR pmps)
{
    char* pvalue = (char*)malloc(pmps->dwLen);
    memset(pvalue, 0xff, pmps->dwLen);
    if(pmps->nType == MAP_TYPE_COM)
    {
        //pmps->dwLen = COM_DATA_SIZE;
        memcpy(gfbp.pCom + pmps->dwStartAddr, pvalue, pmps->dwLen);
    }
    else if(pmps->nType == MAP_TYPE_RAND)
    {
        memcpy(gfbp.pRand + pmps->dwStartAddr, pvalue, pmps->dwLen);
    }
    else if(pmps->nType == MAP_TYPE_RSV)
    {
        /*if(pmps->nInMem == 1)
        memcpy(gfbp.pRsv + pmps->dwStartAddr, pvalue, pmps->dwLen);
        else
        {
        //we shoulde  read it from the file
        //ReadFile(
        }*/
        printf("we should not be here\n");
        //DBG_Getch();DBG_Getch();
    }
    else
    {
        printf("local read should not be here\n");
        //DBG_Getch();DBG_Getch();
    }
    free(pvalue);
    return 1;
}

int Com_Init_Local(unsigned long dwCapacity)
{
    __int64 i64Capacity;
    HANDLE hFile = NULL;
    char* pCom = NULL;
    unsigned long ulSize = 0;
    i64Capacity = dwCapacity * 1024 * 1024 * 1024I64;
    ulSize = i64Capacity / 512;
    ulSize *= COM_DATA_SIZE;
    Construct_Local(ulSize, 1);

    return ulSize;
}
int Rand_Init_Local(unsigned long dwCapacity)
{
    __int64 i64Capacity;
    HANDLE hFile = NULL;
    char* pCom = NULL;
    unsigned long ulSize = 0;
    i64Capacity = dwCapacity * 1024 * 1024 * 1024I64;
    ulSize = i64Capacity / TOBEY_PAGE_SIZE;
    ulSize *= RAND_DATA_SIZE;
    Construct_Local(ulSize, 2);
    return ulSize;
}

int Rsv_Init_Local(unsigned long dwTotalCRand, int nPerRsv)
{
    unsigned long ulSize = MAX_MAP_FILE_SIZE - dwTotalCRand;
    Construct_Local(ulSize, 3);
    gfbp.hRsvFile = CreateFile(RSV_FILE_NAME,               // file to open
        GENERIC_READ | GENERIC_WRITE,          // open for reading
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
        NULL,                  // default security
        OPEN_ALWAYS,         // existing file only
        FILE_ATTRIBUTE_NORMAL, // normal file
        NULL);
    if(INVALID_HANDLE_VALUE == gfbp.hRsvFile)
    {
        printf("rsv file failed\n");
        //DBG_Getch();DBG_Getch();
    }

    return ulSize;
}

int Construct_Local(unsigned long dwSize, int nType)
{
    char* pTmp = NULL;
    HANDLE hFile;
    char* pFileName = NULL;
    HANDLE hFileMapping = NULL;
    pFileName = (char*)malloc(32);
    switch(nType)
    {
    case 1:
        strcpy(pFileName, "com");
        break;
    case 2:
        strcpy(pFileName, "rand");
        break;
    case 3:
        strcpy(pFileName, "rsv");
        break;
    default:
        {
            printf("error rountin\n");
            //DBG_Getch();DBG_Getch();
        }
        break;
    }
    hFile = CreateFile(pFileName,               // file to open
        GENERIC_READ | GENERIC_WRITE,          // open for reading
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
        NULL,                  // default security
        OPEN_ALWAYS,         // existing file only
        FILE_ATTRIBUTE_NORMAL, // normal file
        NULL);                 // no attr. template
    if( INVALID_HANDLE_VALUE == hFile)
    {
        printf("Create file failed\n");
        //DBG_Getch();DBG_Getch();
    }
    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, dwSize, NULL);
    if( INVALID_HANDLE_VALUE == hFile)
    {
        printf("Create file map failed\n");
        //DBG_Getch();DBG_Getch();
    }

    pTmp = (char*)MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, dwSize);
    if(NULL == pTmp)
    {
        printf("Create FileMapping failed\n");
        //DBG_Getch();DBG_Getch();
    }
    switch(nType)
    {
    case 1:
        gfbp.pCom = pTmp;
        gfbp.hFileCom = hFile;
        gfbp.hFileComMapping = hFileMapping;
        break;
    case 2:
        gfbp.hFileRand = hFile;
        gfbp.hFileRandMapping = hFileMapping;
        gfbp.pRand = pTmp;
        break;
    case 3:
        gfbp.hFileRsv = hFile;
        gfbp.hFIleRsvMapping = hFileMapping;
        gfbp.pRsv = pTmp;
        break;
    default:
        {
            printf("error rountin\n");
            //DBG_Getch();DBG_Getch();
        }
        break;
    }
    return 1;
}

int local_un_init()
{
    UnmapViewOfFile(gfbp.pCom);
    CloseHandle(gfbp.hFileCom);
    CloseHandle(gfbp.hFileComMapping);


    UnmapViewOfFile(gfbp.pRand);
    CloseHandle(gfbp.hFileRand);
    CloseHandle(gfbp.hFileRandMapping);


    UnmapViewOfFile(gfbp.pRsv);
    CloseHandle(gfbp.hFileRsv);
    CloseHandle(gfbp.hFIleRsvMapping);

    CloseHandle(gfbp.hRsvFile);
    return 1;
}




//SATA operation
int crs_init_SATA(unsigned long ulCapacity)
{
    int nPSIndex = 0;
    int nTotalSata = 0;
    char PSName[64];
    nTotalSata = Com_Init(ulCapacity, 1);
    gpCPI_Sata = (PCRS_PS_INFO)malloc(nTotalSata * sizeof(CRS_PS_INFO));
    for(;nPSIndex < nTotalSata; nPSIndex++)
    {
        sprintf(PSName, COM_MAPNAME_SATA, nPSIndex);
        gpCPI_Sata[nPSIndex].nType = MAP_TYPE_COM;
        gpCPI_Sata[nPSIndex].bMapped = 0;
        do{
            gpCPI_Sata[nPSIndex].hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, PSName);
        }while(gpCPI_Sata[nPSIndex].hFileMapping == NULL);
    }
    return 1;
}
int crs_read_SATA(PMID_PARA_STR pmpss)
{

    ExeOpe(pmpss, 0x02, 1);
    return 1;
}
int crs_write_SATA(PMID_PARA_STR pmpss)
{
    ExeOpe(pmpss, 0x01, 1);
    return 1;
}
int crs_erase_SATA(PMID_PARA_STR pmpss)
{
    //ExeOpe(pmps, 0x01, 1);
    return 1;
}
int crs_un_init_SATA()
{
    return 1;
}

int local_init_SATA(unsigned long ulCapacity)
{
    __int64 i64Capacity = ulCapacity * 1024 * 1024 * 1024I64;
    int nTotal = i64Capacity / 512;

    DWORD dwOut = 0;
    HANDLE hFileSATA = CreateFile(RSV_FOR_SATA,               // file to open
        GENERIC_READ | GENERIC_WRITE,          // open for reading
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
        NULL,                  // default security
        OPEN_EXISTING,         // existing file only
        FILE_ATTRIBUTE_NORMAL, // normal file
        NULL);

    if(hFileSATA != INVALID_HANDLE_VALUE)
    {
        ReadFile(hFileSATA, &glm, sizeof(LOCAL_MEM), &dwOut, NULL);
        glm.pMem = (char*)malloc(glm.nTotal * sizeof(unsigned long));
        ReadFile(hFileSATA, glm.pMem, glm.nTotal * sizeof(unsigned long), &dwOut, NULL);
    }
    else
    {
        glm.nTotal = nTotal;
        glm.pMem = (char*)malloc((nTotal) * 4);
        memset(glm.pMem, 0, (nTotal) * 4);
        if(glm.pMem == NULL)
        {
            printf("alloc SATA local memory failed");
            DBG_Getch();DBG_Getch();
        }
    }
    return 1;
}
int local_read_SATA(PMID_PARA_STR pmpss)
{
    memcpy(pmpss->pvalue, glm.pMem + pmpss->dwStartAddr, pmpss->dwLen);
    return 1;
}
int local_write_SATA(PMID_PARA_STR pmpss)
{
    memcpy(glm.pMem + pmpss->dwStartAddr, pmpss->pvalue, pmpss->dwLen);
    return 1;
}
int local_erase_SATA(PMID_PARA_STR pmpss)
{
    char* pBuf = (char*)malloc(pmpss->dwLen);
    memset(pBuf, 0, pmpss->dwLen);
    memcpy(glm.pMem + pmpss->dwStartAddr, pBuf, pmpss->dwLen);
    free(pBuf);
    return 1;
}
int local_un_init_SATA()
{
    DWORD dwOut = 0;
    HANDLE hFileSATA = CreateFile(RSV_FOR_SATA,               // file to open
        GENERIC_READ | GENERIC_WRITE,          // open for reading
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
        NULL,                  // default security
        CREATE_ALWAYS,         // existing file only
        FILE_ATTRIBUTE_NORMAL, // normal file
        NULL);
    if(INVALID_HANDLE_VALUE == hFileSATA)
        return 0;
    WriteFile(hFileSATA, &glm, sizeof(LOCAL_MEM), &dwOut, NULL);
    WriteFile(hFileSATA, glm.pMem, sizeof(unsigned long)*glm.nTotal, &dwOut, NULL);

    free(glm.pMem);
    return 1;
}
