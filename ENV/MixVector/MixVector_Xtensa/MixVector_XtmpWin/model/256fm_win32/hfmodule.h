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

#ifndef _H_FMODULE_H
#define _H_FMODULE_H
#include <windows.h>
#include "BaseDef.h"
#include "common_str.h"
#include "model_common.h"
extern unsigned long TOBEY_PAGE_SIZE;
extern unsigned long PU_TOTAL_NUM;
#define RSV_FILE_NAME "rsv.data"
#define RSV_FILE_NAME_CRS "rsvcrs.data"
#define RSV_FOR_SATA "sata.dat"
#define COM_MAPNAME "com%d"
#define COM_MAPNAME_SATA "satacom%d"
#define RAND_MAPNAME "rand%d"
#define RSV_MAPNAME "rsv%d"
#define EXE_FILE_NAME "\\manmem.exe"
#define PS_NUMBER_COM    4
#define ARG_LIST "-N %s -S %u"
#define ONE_RSV_SIZE (1024 * 1024 * 1024I64)

#define MAX_MAP_FILE_SIZE 385 * 1024 * 1024 + 64 * 1024 * 1024
extern unsigned long gulSingleSize_com;
extern unsigned long gulSingleSize_rand;
extern unsigned long gulPSTotal;

extern HANDLE ghRsvFile_crs;
typedef struct _CRS_PS_INFO
{
    int bMapped;
    int nType;
    unsigned long ulStartPos;
    unsigned long ulMappedLen;
    char* pMapped;
    HANDLE hFileMapping;

    //HANDLE hRsvFile;

}CRS_PS_INFO, *PCRS_PS_INFO;
PCRS_PS_INFO gpCPI;
PCRS_PS_INFO gpCPI_Sata;

typedef struct _FILE_BASE_POS
{
    char* pCom;
    char* pRand;
    char* pRsv;

    HANDLE hRsvFile;

    HANDLE hFileCom;
    HANDLE hFileComMapping;

    HANDLE hFileRand;
    HANDLE hFileRandMapping;

    HANDLE hFileRsv;
    HANDLE hFIleRsvMapping;
}FILE_BASE_POS, *PFILE_BASE_POS;
extern FILE_BASE_POS gfbp;

int crs_init(PFLASH_PROPERTY, int nperRsv);
int crs_read(PMID_PARA_STR);
int crs_write(PMID_PARA_STR);
int crs_erase(PMID_PARA_STR);
int crs_un_init(void);


static int Com_Init(unsigned long dwCapacity, int nType);
static int Rand_Init(unsigned long dwCapacity);
static int Rsv_Init(unsigned long dwCapacity, int nPerRsv);
static int CreatePS(char* pArg);
static int PU2CrsPSNum(__int64* i64StartAddr, int nType);
static int ExeOpe(PMID_PARA_STR pmps, int nDirect, int nType);

//local memory operation
int local_init(PFLASH_PROPERTY, int nperRsv);
int local_read(PMID_PARA_STR pmps);
int local_write(PMID_PARA_STR pmps);
int local_erase(PMID_PARA_STR pmps);
int local_un_init(void);

static int Com_Init_Local(unsigned long dwCapacity);
static int Rand_Init_Local(unsigned long dwCapacity);
static int Rsv_Init_Local(unsigned long dwCapacity, int nPerRsv);
static int Construct_Local(unsigned long dwSize, int nType);


//share
void SetMPS(PMID_PARA_STR pmps,int nNum, unsigned long  dwStartAddr,unsigned long  dwLen,int nType,char* pvalue, int nInMem);

//SATA operation 
typedef struct _LOCAL_MEM
{
    int nTotal;
    char* pMem;
}LOCAL_MEM, *PLOCAL_MEM;
LOCAL_MEM glm;

int crs_init_SATA(unsigned long ulCapacity);
int crs_read_SATA(PMID_PARA_STR pmpss);
int crs_write_SATA(PMID_PARA_STR pmpss);
int crs_erase_SATA(PMID_PARA_STR pmpss);
int crs_un_init_SATA();

int local_init_SATA(unsigned long ulCapacity);
int local_read_SATA(PMID_PARA_STR pmpss);
int local_write_SATA(PMID_PARA_STR pmpss);
int local_erase_SATA(PMID_PARA_STR pmpssA);
int local_un_init_SATA();

#endif