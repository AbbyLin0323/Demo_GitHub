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

#ifndef _H_FMID_H
#define _H_FMID_H
#define LOCAL_PROCESS_MEMORY 0x01
#define CROSS_PROCESS_MEMORY 0x02


#define ONE_PU_FILE_SIZE(gfp) (gfp.gnHowManyPln * gfp.gnHowManyBlk * gfp.gnHowManyPge)
#define ONE_PLN_FILE_SIZE(gfp) (gfp.gnHowManyBlk * gfp.gnHowManyPge)
#define ONE_BLK_FILE_SIZE(gfp) (gfp.gnHowManyPge)

#define EMPTY 0
#define FULL 0
#define NOT_EMPTY_FULL 1
#define NO_INDEX 0xFFFFFFFF

#include "model_common.h"
#include "common_str.h"
#define SAVE_FILE_NAME "log.dat"

#ifdef    __cplusplus
extern "C" {
#endif
int COM_DATA_SIZE;

typedef struct _FLASH_PHY
{
    union
    {
        struct
        {
            UINT16 bsPhyPu : 8;
            UINT16 bsLunInCe : 8;
            UINT8 nPU;
            UINT8 nPln;
            UINT16 nBlock;
            UINT16 nPage;
        };

        UINT64 u64Addr;
    };

}FLASH_PHY, *PFLASH_PHY;

typedef struct _PGE_INFO
{
    int nTotal;
    unsigned long*pPgeInfo;
}PGE_INFO, *PPGEINFO;

typedef struct _BUF_MANAGE_RSV
{
    int nTotal;
    unsigned long* pBuf;//Save the memory index

    int nHead;
    int nTail;
    unsigned long ulCurNum;
}BUF_MANAGE_RSV, *PBUF_MANAGE_RSV;

typedef struct _FILE_MANAGE_RSV
{
    unsigned long* pRsvIndex;
    int nTop;
    int nHWRsvIndex;
}FILE_MANAGE_RSV, *PFILE_MANAGE_RSV;

FILE_MANAGE_RSV gFMR;


BUF_MANAGE_RSV gBMR;
PGE_INFO gPgeInfo;
unsigned long TOBEY_PAGE_SIZE;
unsigned long PU_TOTAL_NUM;

void SetFun(PMID_LEVEL_STR pMls, int nType);

void Mid_Init(int nPu, int nPln, int nBlk, int nPge, unsigned long ulCapacity, unsigned long ulPgeSize, int nRsvPer);
void Mid_Read(PFLASH_PHY pFlash_phy, int nType, char* pBuf, int nLength);
void Mid_Write(PFLASH_PHY pFlash_phy, int nType, char* pBuf, int nLength);
U8 Mid_Erase(PFLASH_PHY pFlash_phy);
void Mid_Un_Init(void);
static __int64 FPos2MPos(PFLASH_PHY pFlash_phy, int nType);
static __int64 FPos2MPos_Com(PFLASH_PHY pFlash_phy);
static __int64 FPos2MPos_Rand(PFLASH_PHY pFlash_phy);
static __int64 FPos2MPos_Rsv(PFLASH_PHY pFlash_phy);
static int Dec_interface(unsigned long ulCapacity);


//Buf manage
extern int Empty(PBUF_MANAGE_RSV pBMR);
extern unsigned long  GetOne(PBUF_MANAGE_RSV pBMR);
extern int RsyOne(PBUF_MANAGE_RSV pBMR, unsigned long);
extern int Full(PBUF_MANAGE_RSV pBMR);

//Rsv file manage
extern void Init_FMR(unsigned long rsvFile, int nPgeSize);
extern unsigned long Get_FMR();
extern void Set_FMR(unsigned long rsvFIndex);
extern void Un_init_FMR();

//Sata Interface
void Mid_Init_SATA(unsigned long ulCapacity);
void Mid_Read_SATA(unsigned long ulLBA, char* pBuf, int nLength);
void Mid_Write_SATA(unsigned long ulLBA, char* pBuf, int nLength);
void Mid_Erase_SATA(unsigned long ulLBA);
void MID_Un_Init_SATA();

//error check
static int Check(int nPU, int nPln, int nBlk, int nPge, PFLASH_PHY pFP);

#ifdef    __cplusplus
}
#endif

#endif