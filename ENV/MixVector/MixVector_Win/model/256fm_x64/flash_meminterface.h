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


int crs_un_init(void);

#define COM_DATA_TYPE 0x01
#define RAND_DATA_TYPE 0x02
#define RSV_DATA_TYPE 0x04

typedef struct _FLASH_PHY
{
    union
    {
        struct 
        {
            UINT16 nPU;
            UINT16 nPln;
            UINT16 nBlock;
            UINT16 nPage;
        };

        UINT64 u64Addr;
    };
}FLASH_PHY,*PFLASH_PHY;

#define FLASH_OP_LOG_MAX    1000
typedef struct _FLASH_OP_LOG_ENTRY
{
    FLASH_PHY FlashAddr;
    U32 DataLPN[8];
    char      FlashOP;

}FLASH_OP_LOG_ENTRY;

typedef struct _FLASH_OP_LOG
{
    FLASH_OP_LOG_ENTRY FlashOpLog[FLASH_OP_LOG_MAX];
    U32 FlashLogIndex;
}FLASH_OP_LOG;

// sata interface
int Mid_init_SATA();
//void Mid_Read_SATA(unsigned long ulLBA, char* pBuf, int nLength);
void Mid_Read_SATA(UINT32 ulLBA, char* pBuf, int nLength);
void Mid_Write_SATA(unsigned long ulLBA, char* pBuf, int nLength);
void Mid_Erase_SATA(unsigned long ulLBA);
//void MID_Un_Init_SATA();

// flash interface
void Mid_Init_Ex();
//void Mid_Init(int nPu, int nPln, int nBlk, int nPge, unsigned long ulCapacity, unsigned long ulPgeSize, int nRsvPer);
void Mid_Read(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nDataLen);
void Mid_Write(PFLASH_PHY pFlash_phy, int nType, char* pDataBuf, char * pRedBuf, int nLength);
void Mid_Erase(PFLASH_PHY pFlash_phy);
void Mid_Read_RedData(PFLASH_PHY pFlash_phy, char *pRedBuf);

#endif