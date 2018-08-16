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

#ifndef _COMMON_STR
#define _COMMON_STR

typedef struct _FLASH_PROPERTY
{
    int gnHowManyPU;
    int gnHowManyPln;
    int gnHowManyBlk;
    int gnHowManyPge;
    int gnCapacity;
    int gnPageType;
}FLASH_PROPERTY, *PFLASH_PROPERTY;

//share parameter for local and cross process
typedef struct _MID_PARA_STR
{
    int nNum;
    unsigned long  dwStartAddr;
    unsigned long dwStartAddrHigh;
    unsigned long  dwLen;
    unsigned int nInMem;//The value should include the flag that whether the data is saved in the memory or not, just for the rsv
    int nType;
    char* pvalue;
}MID_PARA_STR, *PMID_PARA_STR;

//share parameter for local and cross process

extern int COM_DATA_SIZE;
#define RAND_DATA_SIZE 64

#define IN_MEMORY 0x80000000
#define IN_FILE 0x40000000
#define IN_NULL 0x0
#define MEMFILE_MASK 0xC0000000
#define NOT_FOUND ~MEMFILE_MASK

typedef struct _MID_LEVEL_STR
{
    int (*Init)(PFLASH_PROPERTY, int nperRsv);
    int (*Read)(PMID_PARA_STR);
    int (*Write)(PMID_PARA_STR);
    int (*Erase)(PMID_PARA_STR);
    int(*UnInit)(void);

    int m_nType;
    FLASH_PROPERTY m_fp;//Include the flash property.
}MID_LEVEL_STR, *PMID_LEVEL_STR;
MID_LEVEL_STR gmls;

typedef struct _MID_LEVEL_STR_SATA
{
    int (*Init)(unsigned long  dwCapacity);
    int (*Read)(PMID_PARA_STR);
    int (*Write)(PMID_PARA_STR);
    int (*Erase)(unsigned long ulLBA);
    int (*UnInit)();

    int m_nType;
}MID_LEVEL_STR_SATA, *PMID_LEVEL_STR_SATA;
MID_LEVEL_STR_SATA gmlss;

#define COM_DATA_TYPE 0x01
#define RAND_DATA_TYPE 0x02
#define RSV_DATA_TYPE 0x04

#define MAP_TYPE_COM COM_DATA_TYPE
#define MAP_TYPE_RAND RAND_DATA_TYPE
#define MAP_TYPE_RSV RSV_DATA_TYPE

#define RSV_FILE_SIZE 8
#endif