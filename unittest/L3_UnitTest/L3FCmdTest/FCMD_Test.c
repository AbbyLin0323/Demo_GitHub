/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc. and may  *
* contain trade secrets and/or other confidential information of VIA           *
* Technologies, Inc. This file shall not be disclosed to any third party, in   *
* whole or in part, without prior written consent of VIA.                      *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : FCMD_Test.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2014.12.9
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_MultiCore.h"
#include "HAL_FlashDriverBasic.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "L2_FTL.h"
#include "L2_Boot.h"
#include "L2_PBIT.h"
#include "L2_FCMDQ.h"
#include "L2_TableBlock.h"
#include "L2_TableBBT.h"
#include "FCMD_Test.h"
#include "L2_Interface.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
// Only for FCmd Test State Management
typedef enum _FCMD_TEST_STATE
{
    FCMD_TEST_STATE_INIT = 0,
    FCMD_TEST_STATE_RUN,
    FCMD_TEST_STATE_SAVEBBT,
    FCMD_TEST_STATE_DONE
}FCMD_TEST_STATE;

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
LOCAL U32 l_ulL3UTDummyDataAddr;
LOCAL U32 l_ulL3UTDummyDataAddrFor2DTlcWrite;
LOCAL U32 l_ulL3UTDummySpareAddr;
LOCAL U32 l_ulL3UTDummySpareAddrFor2DTlcWrite;
LOCAL U32 l_ulL3UTDummyStatusAddr;
LOCAL U32 l_ulL3UTDummyStatusAddrOTFB;

//all the Lpn test case, and we assume that the case number is less than PG_PER_BLK
LOCAL U32 l_ulMaxLpnRdCase;
LOCAL L3_UT_LPN_READ_CASE *l_aUTLpnRdCase;
//for 2 plane flash, 2 case in first plane, 2 cross plane, 2 in second plane
#define PARTIAL_CASE_NUM    6
LOCAL L3_UT_NORMAL_READ_CASE *l_aPartialRdCase;
LOCAL U32 l_ulRandSeed;
LOCAL U32 l_ulSubsysNumBits;
LOCAL U8 l_aFCmdPtr[SUBSYSTEM_LUN_MAX];
LOCAL U8 l_ucLPNCaseOffset = 0;
LOCAL U32 l_aLunInSPUBitMap[SUBSYSTEM_SUPERPU_MAX][BITMAP_GRP_NUM_PER_SPU] = { 0 };

static U32 s_ulTotalPrgOrder[SUBSYSTEM_LUN_MAX] = { 0 };
static U32 s_ulStartCheckCnt[SUBSYSTEM_LUN_MAX] = { 0 };

GLOBAL L3_UT_PU_MG_DPTR *g_ptPuMgDptr;


LOCAL void L3_UTResetTotalPrgOrder(U8 ucTLun)
{
    s_ulTotalPrgOrder[ucTLun] = 0;
    return;
}

GLOBAL L3_UT_LPN_READ_CASE * MCU12_DRAM_TEXT L3_GetLPNCase()
{
    return l_aUTLpnRdCase;
}

GLOBAL U8 MCU12_DRAM_TEXT L3_GetLPNCaseOffset()
{
    return l_ucLPNCaseOffset;
}

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_UTSrand
Input      : U32 Seed
Output     : None
Return Val :
Discription: randomizer engine - seed
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L3_UTSrand(U32 Seed)
{
    l_ulRandSeed = Seed;
}

/*==============================================================================
Func Name  : L3_UTRand
Input      : None
Output     : None
Return Val :
Discription: randomizer engine - get a rand number.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
S32 L3_UTRand(void)
{
    const S32 m = 2147483399;
    const S32 a = 40692;
    const S32 q = m/a;
    const S32 r = m%a;

    l_ulRandSeed = a * (l_ulRandSeed%q) - r* (l_ulRandSeed/q);
    if (l_ulRandSeed<0)
    {
        l_ulRandSeed += m;
    }

    return l_ulRandSeed & 0x7fff;
}

/*==============================================================================
Func Name  : L3_UTGetDataBuffAddr
Input      : U8 Pu
Output     : None
Return Val :
Discription: get the Data Buffer Adress of the current Local Flash Request.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
U32 L3_UTGetDataBuffAddr(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulL3UTDummyDataAddr + (NFCQ_DEPTH*ucTLun + ulWPtr)*LOGIC_PIPE_PG_SZ;

    return ulBuffAddr;
}
U32 L3_UTGetDataBuffAddrFor2DTlcWrite(U8 ucTLun, U16 usPrgWL, U8 ucPageIndex)
{
    U32 ulBuffAddr;

    //ulBuffAddr = l_ulL3UTDummyDataAddrFor2DTlcWrite + (DSG_BUFF_SIZE*(PG_PER_WL + 1)*ucTLun + (usPrgWL % (INTRPG_PER_PGADDR * PGADDR_PER_PRG + 1))*DSG_BUFF_SIZE + ucPageIndex)*LOGIC_PIPE_PG_SZ;
    ulBuffAddr = l_ulL3UTDummyDataAddrFor2DTlcWrite + (2 * NFCQ_DEPTH * ucTLun + 2* (usPrgWL % NFCQ_DEPTH) + ucPageIndex)*LOGIC_PIPE_PG_SZ;

    return ulBuffAddr;
}
/*==============================================================================
Func Name  : L3_UTGetSpareBuffAddr
Input      : U8 Pu
Output     : None
Return Val :
Discription: get the Spare Buffer Adress of the current Flash Request.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
U32 L3_UTGetSpareBuffAddr(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulL3UTDummySpareAddr + (NFCQ_DEPTH*ucTLun + ulWPtr)*sizeof(RED)*PG_PER_WL;

    return ulBuffAddr;
}
U32 L3_UTGetSpareBuffAddrFor2DTlcWrite(U8 ucTLun, U16 usPrgWL, U8 ucPageIndex)
{
    U32 ulBuffAddr;

    //ulBuffAddr = l_ulL3UTDummySpareAddrFor2DTlcWrite + (DSG_BUFF_SIZE*(PG_PER_WL + 1)*ucTLun + (usPrgWL % (PG_PER_WL + 1))*DSG_BUFF_SIZE + ucPageIndex)*sizeof(RED);
    ulBuffAddr = l_ulL3UTDummySpareAddrFor2DTlcWrite + (2 * NFCQ_DEPTH * ucTLun + 2 * (usPrgWL%NFCQ_DEPTH) + ucPageIndex)*sizeof(RED);

    return ulBuffAddr;
}

/*==============================================================================
Func Name  : L3_UTGetStatusBuffAddr
Input      : U8 Pu
Output     : None
Return Val :
Discription: get the FlashStatus Buffer Adress of the current Flash Request.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL U32 L3_UTGetStatusBuffAddr(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulL3UTDummyStatusAddr + (NFCQ_DEPTH*ucTLun + ulWPtr)*sizeof(U8);

    return ulBuffAddr;
}

U32 L3_UTGetStatusBuffAddrOTFB(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulL3UTDummyStatusAddrOTFB + (NFCQ_DEPTH*ucTLun + ulWPtr)*sizeof(U8);

    return ulBuffAddr;
}

/*==============================================================================
Func Name  : L3_UTCalcLPNCount
Input      : U32 SecStart, U32 SecLen
Output     : None
Return Val :
Discription: calc the lpn number by SecStart & SecLen.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL U8 MCU1_DRAM_TEXT L3_UTCalcLPNCount(U32 SecStart, U32 SecLen)
{
    U8 ucLpnCnt=0, ucSecLenInLpn;

    if(0 < SecLen)
    {
        if(SecStart%SEC_PER_LPN)
        {
            ucLpnCnt++;
            ucSecLenInLpn = SEC_PER_LPN - (SecStart%SEC_PER_LPN);
            SecLen -= (SecLen >= ucSecLenInLpn) ? ucSecLenInLpn : SecLen;
        }

        while(0 < SecLen)
        {
            ucLpnCnt++;
            SecLen -= (SecLen >= SEC_PER_LPN) ? SEC_PER_LPN : SecLen;
        }
    }

    return ucLpnCnt;
}

/*==============================================================================
Func Name  : L3_UTInitFCmdPtrForDataCheck
Input      : U8 Pu
Output     : None
Return Val :
Discription: L3 Unit Test self-check.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
void L3_UTInitFCmdPtrForDataCheck(void)
{
    U8 ucTLun;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        l_aFCmdPtr[ucTLun] = INVALID_2F;
    }

    return;
}

FCMD_REQ_ENTRY *L3_UTGetEntryForDataCheck(U8 ucTLun)
{

    U8 ucCurPtr;

    ucCurPtr = (INVALID_2F == l_aFCmdPtr[ucTLun]) ? L2_FCMDQGetReqWptr(ucTLun, 0) : l_aFCmdPtr[ucTLun];
    if (INVALID_2F == l_aFCmdPtr[ucTLun])
    {
        s_ulStartCheckCnt[ucTLun]++;
        if (s_ulStartCheckCnt[ucTLun] >= NFCQ_DEPTH)
        {
            l_aFCmdPtr[ucTLun] = (ucCurPtr + NFCQ_DEPTH + 1)%NFCQ_DEPTH;
        }
        return NULL;
    }

    if (TRUE != L2_FCMDQIsWptrFree(ucTLun,0, ucCurPtr))
    {
        return NULL;
    }

    l_aFCmdPtr[ucTLun] = (ucCurPtr + 1) % NFCQ_DEPTH;

    return L2_FCMDQGetReqEntry(ucTLun, 0, ucCurPtr);
}

/*==============================================================================
Func Name  : L3_UTDataCheck
Input      : U8 Pu
Output     : NONE
Return Val :
Discription: Read command -> Check Data and Red when read success.
Usage      :
History    :
    1. 2016.3.19 JasonGuo create function
==============================================================================*/
void L3_UTDataCheck(U8 ucTLun)
{
    U32 ulDummyRed, ulRealRed, ulDummyData, ulRealData;
    U32 ulSecTestNum, ulReadDataBuffAddr, ulSecIndx;
    FCMD_REQ_ENTRY *ptFCmdEntry;

#ifndef UTC_DATA_CHECK_ENABLE
    return;
#endif

    // get the checked fcmd-entry and pointed the next checked fcmd-entry.
    ptFCmdEntry = L3_UTGetEntryForDataCheck(ucTLun);
    if (NULL == ptFCmdEntry)
    {
        return;
    }
    //DBG_Printf("Lun%d Blk%d Page%d Check.\n", ptFCmdEntry->bsTLun, ptFCmdEntry->tFlashDesc.bsVirBlk, ptFCmdEntry->tFlashDesc.bsVirPage);

    // check read only
    if ((FCMD_REQ_TYPE_READ == ptFCmdEntry->bsReqType)
     && (SUBSYSTEM_STATUS_SUCCESS == *(U8*)ptFCmdEntry->ulReqStsAddr
     || SUBSYSTEM_STATUS_RECC == *(U8*)ptFCmdEntry->ulReqStsAddr
     || SUBSYSTEM_STATUS_RETRY_SUCCESS == *(U8*)ptFCmdEntry->ulReqStsAddr))
    {
        if (FALSE == ptFCmdEntry->tFlashDesc.bsRdRedOnly)
        {
            ulReadDataBuffAddr = COM_GetMemAddrByBufferID(ptFCmdEntry->atBufDesc[0].bsBufID, TRUE, BUF_SIZE_BITS);
            if (FALSE == ptFCmdEntry->tFlashDesc.bsMergeRdEn)
            {
                ulSecTestNum = ptFCmdEntry->tFlashDesc.bsSecLen;
                for (ulSecIndx = 0; ulSecIndx < ulSecTestNum; ulSecIndx++)
                {
                    ulDummyData = UT_GET_DUMMY_DATA(ptFCmdEntry->bsTLun, ptFCmdEntry->tFlashDesc.bsVirBlk, ptFCmdEntry->tFlashDesc.bsVirPage, ptFCmdEntry->tFlashDesc.bsSecStart + (ptFCmdEntry->tFlashDesc.bsPlnNum * SEC_PER_BUF / PLN_PER_LUN) + ulSecIndx);
                    ulRealData = *((volatile U32 *)(ulReadDataBuffAddr + (ptFCmdEntry->atBufDesc[0].bsSecStart + ulSecIndx)*SEC_SIZE) + 1);
                    #ifdef SIM
                    if (ulDummyData != ulRealData && INVALID_8F != ulRealData)
                    #else
                    if (ulDummyData != ulRealData &&  0 != ulRealData)
                    #endif
                    {
                        DBG_Printf("MCU#%d [%d,%d,%d] Read Data Error, RealData=0x%x,DummyData=0x%x\n",
                            HAL_GetMcuId(), ptFCmdEntry->bsTLun, ptFCmdEntry->tFlashDesc.bsVirBlk, ptFCmdEntry->tFlashDesc.bsVirPage, ulRealData, ulDummyData);
                        DBG_Getch();
                    }
                }
            }
            else
            {
                U32 ulLpnBitMap = ptFCmdEntry->tFlashDesc.bsLpnBitmap;
                U32 ulBitPos = 0;

                while (0 != ulLpnBitMap)
                {
                    if (0 != (ulLpnBitMap & (1 << ulBitPos)))
                    {
                        ulSecIndx = ulBitPos;

                        ulDummyData = UT_GET_DUMMY_DATA(ptFCmdEntry->bsTLun, ptFCmdEntry->tFlashDesc.bsVirBlk, ptFCmdEntry->tFlashDesc.bsVirPage, ptFCmdEntry->tFlashDesc.bsSecStart + (ptFCmdEntry->tFlashDesc.bsPlnNum * SEC_PER_BUF / PLN_PER_LUN) + ulSecIndx);
                        ulRealData = *((U32 *)(ulReadDataBuffAddr + (ptFCmdEntry->atBufDesc[0].bsSecStart + ulSecIndx)*SEC_SIZE) + 1);
                        #ifdef SIM
                        if (ulDummyData != ulRealData && INVALID_8F != ulRealData)
                        #else
                        if (ulDummyData != ulRealData && 0 != ulRealData)
                        #endif
                        {
                            DBG_Printf("MCU#%d [%d,%d,%d] Merge Read Data Error, RealData=0x%x, Addr=0x%x, DummyData=0x%x\n",
                                HAL_GetMcuId(), ptFCmdEntry->bsTLun, ptFCmdEntry->tFlashDesc.bsVirBlk, ptFCmdEntry->tFlashDesc.bsVirPage,
                                ulRealData, (ulReadDataBuffAddr + (ptFCmdEntry->atBufDesc[0].bsSecStart + ulSecIndx)*SEC_SIZE),ulDummyData);
                            DBG_Getch();
                        }
                    }

                    ulLpnBitMap &= ~(1 << ulBitPos);

                    ulBitPos++;
                }
            }
        }

        if (0 != ptFCmdEntry->ulSpareAddr)
        {
            #ifdef EM_LBA_CHKEN
            ulDummyRed = L3_UT_DUMMY_RED(0,0,1);
            #else
            ulDummyRed = UT_GET_DUMMY_DATA(ptFCmdEntry->bsTLun, ptFCmdEntry->tFlashDesc.bsVirBlk, ptFCmdEntry->tFlashDesc.bsVirPage, 1);
            #endif
            ulRealRed  = *((U32 *)ptFCmdEntry->ulSpareAddr);
            if (ulDummyRed != ulRealRed)
            {
                DBG_Printf("MCU#%d [%d,%d,%d] Red Error, RealRed=0x%x, DummyRed=0x%x\n",
                    HAL_GetMcuId(), ptFCmdEntry->bsTLun, ptFCmdEntry->tFlashDesc.bsVirBlk, ptFCmdEntry->tFlashDesc.bsVirPage, ulRealRed, ulDummyRed);
                DBG_Getch();
            }
        }
    }

    return;
}

/*==============================================================================
Func Name  : V2L3_UTDramVarInit
Input      : U32 *pFreeDramBase
Output     : None
Return Val :
Discription: L3 Unit Test var init.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L3_UTDramVarInit(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase = *pFreeDramBase;

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulL3UTDummyDataAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, LOGIC_PIPE_PG_SZ*SUBSYSTEM_LUN_NUM*NFCQ_DEPTH);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulL3UTDummyDataAddrFor2DTlcWrite = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, LOGIC_PIPE_PG_SZ*SUBSYSTEM_LUN_NUM*DSG_BUFF_SIZE*(PG_PER_WL + 1));

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulL3UTDummySpareAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RED)*PG_PER_WL*SUBSYSTEM_LUN_NUM*NFCQ_DEPTH);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulL3UTDummySpareAddrFor2DTlcWrite = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RED)*SUBSYSTEM_LUN_NUM*DSG_BUFF_SIZE*(PG_PER_WL + 1));

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_ulL3UTDummyStatusAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(U8)*SUBSYSTEM_LUN_NUM*NFCQ_DEPTH);

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_aUTLpnRdCase = (L3_UT_LPN_READ_CASE *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(L3_UT_LPN_READ_CASE)*LOGIC_PG_PER_BLK*PG_PER_WL);

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_aPartialRdCase = (L3_UT_NORMAL_READ_CASE *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(L3_UT_NORMAL_READ_CASE)*PARTIAL_CASE_NUM);

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    g_ptPuMgDptr = (L3_UT_PU_MG_DPTR *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(L3_UT_PU_MG_DPTR)*SUBSYSTEM_LUN_NUM);

    DBG_Printf("MCU#%d UT_FCMDTest Alloc Dram [0x%x,0x%x] Size: 0x%x\n",
    HAL_GetMcuId(),*pFreeDramBase, ulFreeDramBase, ulFreeDramBase-*pFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;

    return;
}

/*==============================================================================
Func Name  : L3_UTOtfbVarInit
Input      : U32 *pFreeOtfbBase
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2014.12.18 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L3_UTOtfbVarInit(U32 *pCacheStatusBase)
{
    U32 ulCacheStatusOTFBBase = *pCacheStatusBase;

    COM_MemAddr16DWAlign(&ulCacheStatusOTFBBase);
    l_ulL3UTDummyStatusAddrOTFB = ulCacheStatusOTFBBase;
    COM_MemIncBaseAddr(&ulCacheStatusOTFBBase, sizeof(U8)*SUBSYSTEM_LUN_NUM*NFCQ_DEPTH);

    *pCacheStatusBase = ulCacheStatusOTFBBase;

    return;
}

/*==============================================================================
Func Name  : L3_UTMgDptrInit
Input      : None
Output     : None
Return Val :
Discription: L3 Unit Test Management controller init.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L3_UTMgDptrInit(void)
{
    U8 ucTLun;

    COM_MemZero((U32*)&l_aLunInSPUBitMap[0][0], SUBSYSTEM_SUPERPU_MAX*BITMAP_GRP_NUM_PER_SPU);

    COM_MemZero((U32*)g_ptPuMgDptr, sizeof(L3_UT_PU_MG_DPTR)*SUBSYSTEM_LUN_NUM/sizeof(U32));
    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        g_ptPuMgDptr[ucTLun].BurnCnt = 0;
        g_ptPuMgDptr[ucTLun].CurStage = L3_UT_LOCAL_STAGE_INIT;
        g_ptPuMgDptr[ucTLun].FlashSLCMode = FALSE; // default tlc operation
        g_ptPuMgDptr[ucTLun].FlashSingePln = FALSE; // default multi-pln operation
        g_ptPuMgDptr[ucTLun].Blk = 455;// L3_UTRand() % BLK_PER_LUN;

        #ifdef UTC_UN_CLOSED_BLK_WRITE_ENABLE
        //g_ptPuMgDptr[ucTLun].LastPage = L3_UTRand() % ((FALSE == g_ptPuMgDptr[ucTLun].FlashSLCMode) ? LOGIC_PG_PER_BLK * PG_PER_WL : PG_PER_SLC_BLK); // control the write page number
        #else
        //g_ptPuMgDptr[ucTLun].LastPage = ((FALSE == g_ptPuMgDptr[ucTLun].FlashSLCMode) ? LOGIC_PG_PER_BLK*PG_PER_WL : PG_PER_SLC_BLK) - 1;
        #endif
    }

    return;
}

/*==============================================================================
Func Name  : L3_UTLpnReadCaseInit
Input      : None
Output     : None
Return Val :
Discription: init Lpn Read Case.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L3_UTLpnReadCaseInit(void)
{
    U32 i,j,k,ulCaseNum;

    ulCaseNum = 0;
    for(i=0; i<LPN_PER_BUF; i++)
    {
        for(j=i; j<LPN_PER_BUF; j++)
        {
            for(k=0; k<LPN_PER_BUF-(j-i); k++)
            {
                l_aUTLpnRdCase[ulCaseNum].Cnt = j-i+1;
                l_aUTLpnRdCase[ulCaseNum].StartInPage=i;
                l_aUTLpnRdCase[ulCaseNum].StartInBuf=k;
                ulCaseNum++;
            }
        }
    }

    l_ulMaxLpnRdCase = ulCaseNum;

    return;
}

/*==============================================================================
Func Name  : L3_UTPartialReadCaseInit
Input      : None
Output     : None
Return Val :
Discription: init Partial Read Case.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L3_UTPartialReadCaseInit(void)
{
    U32 i,j;
    U32 ulSecPerPage, ulSecStart, ulSecEnd, ulTmp;

    ulSecPerPage = SEC_PER_BUF>>PLN_PER_LUN_BITS;

    for(i=0; i<PARTIAL_CASE_NUM; i++)
    {
        ulSecStart = L3_UTRand() % ulSecPerPage;
        ulSecEnd   = L3_UTRand() % ulSecPerPage;

        if (i < 2) // 2 case in first plane
        {
            if(ulSecStart > ulSecEnd)
            {
                ulTmp = ulSecStart, ulSecStart = ulSecEnd, ulSecEnd = ulTmp;
            }
        }
        else if (i < 4) // 2 case in cross plane
        {
            ulSecEnd  += ulSecPerPage;
        }
        else // 2 case in second plane
        {
            if(ulSecStart > ulSecEnd)
            {
                ulTmp = ulSecStart, ulSecStart = ulSecEnd, ulSecEnd = ulTmp;
            }
            ulSecStart += ulSecPerPage;
            ulSecEnd   += ulSecPerPage;
        }

        l_aPartialRdCase[i].ReqOffset = ulSecStart;
        l_aPartialRdCase[i].ReqLength = ulSecEnd-ulSecStart+1;
        l_aPartialRdCase[i].LPNOffset = ulSecStart/SEC_PER_LPN;
        l_aPartialRdCase[i].LPNCount  = L3_UTCalcLPNCount(ulSecStart,l_aPartialRdCase[i].ReqLength);

        for(j=l_aPartialRdCase[i].LPNOffset; j<l_aPartialRdCase[i].LPNOffset+l_aPartialRdCase[i].LPNCount; j++)
        {
            l_aPartialRdCase[i].AddrArray[j].m_LPNInPage = j;
        }
    }

    return;
}

GLOBAL void L3_NormalReadPage(FCMD_REQ_ITEM *ptFCmdReqItem, U16 *pBuffID, FCMD_READ_RANGE *ptRange, U8 *pStatus)
{
    FCMD_REQ_ENTRY *ptReqEntry;

    ptReqEntry = L2_FCMDQAllocReqEntry(ptFCmdReqItem->bsTLun, 0);
    ptReqEntry->bsReqType = ptFCmdReqItem->bsFCmdType;
    ptReqEntry->bsReqSubType = ptFCmdReqItem->bsFCmdSubType;
    ptReqEntry->bsTableReq = ptFCmdReqItem->bsTableReq;

    ptReqEntry->tFlashDesc.bsVirBlk = ptFCmdReqItem->bsBlk;
    ptReqEntry->tFlashDesc.bsVirPage = ptFCmdReqItem->bsPage;
    ptReqEntry->tFlashDesc.bsPlnNum = ptFCmdReqItem->bsPln;
    ptReqEntry->tFlashDesc.bsBlkMod = (TRUE==ptFCmdReqItem->bsSLCMode)? FCMD_REQ_SLC_BLK : FCMD_REQ_TLC_BLK;
    ptReqEntry->tFlashDesc.bsShiftRdEn = ptFCmdReqItem->bsShiftReadEn;

    ptReqEntry->tFlashDesc.bsSecStart = ptRange->bsLPNInPage * SEC_PER_LPN;
    ptReqEntry->tFlashDesc.bsSecLen = ptRange->bsLPNCnt * SEC_PER_LPN;

    // Whether it needs to be split, just valid for GC
    if ((ptRange->bsLPNInBuff + ptRange->bsLPNCnt) > LPN_PER_BUF)
    {
        ptReqEntry->atBufDesc[0].bsBufID = pBuffID[0];
        ptReqEntry->atBufDesc[0].bsSecStart = ptRange->bsLPNInBuff * SEC_PER_LPN;
        ptReqEntry->atBufDesc[0].bsSecLen = (LPN_PER_BUF - ptRange->bsLPNInBuff) * SEC_PER_LPN;
        ptReqEntry->atBufDesc[1].bsBufID = pBuffID[1];
        ptReqEntry->atBufDesc[1].bsSecStart = 0;
        ptReqEntry->atBufDesc[1].bsSecLen = (ptRange->bsLPNInBuff + ptRange->bsLPNCnt - LPN_PER_BUF) * SEC_PER_LPN;
    }
    else
    {
        ptReqEntry->atBufDesc[0].bsBufID = pBuffID[0];
        ptReqEntry->atBufDesc[0].bsSecStart = ptRange->bsLPNInBuff * SEC_PER_LPN;
        ptReqEntry->atBufDesc[0].bsSecLen = ptRange->bsLPNCnt * SEC_PER_LPN;
        ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
    }

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ptFCmdReqItem->bsTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return;
}

/*==============================================================================
Func Name  : L3_UTSelectLunForPush
Input      : U8 ucSPU
Output     : NONE
Return Val : GLOBAL
Discription: Simulate the L2 to select a Lun for send fcmd.
Usage      :
History    :
1. 2016.3.24 JasonGuo create function
==============================================================================*/
LOCAL void L3_UTUpdateLunInSPUBitMap(U8 ucSPU)
{
    U8 ucTLun;
    LOCAL U8 ucLunInSPU[SUBSYSTEM_SUPERPU_MAX] = { 0 };

    ucLunInSPU[ucSPU] %= LUN_NUM_PER_SUPERPU;

    for (; ucLunInSPU[ucSPU] < LUN_NUM_PER_SUPERPU; ucLunInSPU[ucSPU]++)
    {
        ucTLun = L2_GET_TLUN(ucSPU, ucLunInSPU[ucSPU]);
        if (TRUE == L2_FCMDQNotFull(ucTLun))
        {
            l_aLunInSPUBitMap[ucSPU][ucLunInSPU[ucSPU] / 32] |= (1 << (ucLunInSPU[ucSPU] % 32));
            //break;
        }
    }

    return;
}
LOCAL U8 L3_UTSelectLunInSPU(U8 ucSPU)
{
    U8 ucIndex, ucLunInSuperPu;

    for (ucIndex = 0; ucIndex < BITMAP_GRP_NUM_PER_SPU; ucIndex++)
    {
        ucLunInSuperPu = HAL_CLZ(l_aLunInSPUBitMap[ucSPU][ucIndex]);
        if (32 != ucLunInSuperPu)
        {
            l_aLunInSPUBitMap[ucSPU][ucIndex] &= ~(1 << (31 - ucLunInSuperPu));
            return (ucIndex * 32) + (31 - ucLunInSuperPu);
        }
    }

    return INVALID_2F;
}
U8 L3_UTSelectLunForPush(U8 ucSPU)
{
    U8 ucLunInSuperPu;

    ucLunInSuperPu = L3_UTSelectLunInSPU(ucSPU);
    if (INVALID_2F == ucLunInSuperPu)
    {
        L3_UTUpdateLunInSPUBitMap(ucSPU);
        ucLunInSuperPu = L3_UTSelectLunInSPU(ucSPU);
    }

    return ucLunInSuperPu;
}

U8 L3_UTSelectNextStage(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucCurStage)
{
    U8 ucNextStage;
    U16 usPhyBlk;
    U8 ucTLun;
    U32 ulPageNumPerBlkForTest;
    L3_UT_PU_MG_DPTR *ptPuDptr;

    BOOL bRandomReadEn = FALSE;
    #ifdef UTC_RANDOM_READ_MODE
    bRandomReadEn = TRUE;
    #endif


    /*convert [ucSuperPu:ucLunInSuperPu] to [ucPu:ucBLun]*/
    ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

    /*obtain ucPu's manaer and total test page num*/
    ptPuDptr = &g_ptPuMgDptr[ucTLun];
    ulPageNumPerBlkForTest = ptPuDptr->LastPage+1;

    if (L3_UT_LOCAL_STAGE_INIT == ucCurStage)
    {
        if (0 == ptPuDptr->SLCBlkStart)
        {
            while (TRUE == L2_VBT_Get_TLC(ucSuperPu, ptPuDptr->Blk) || ptPuDptr->Blk < TABLE_BLOCK_COUNT)
            {
                ptPuDptr->Blk = (ptPuDptr->Blk + 1) % BLK_PER_LUN;
            }
            ptPuDptr->SLCBlkStart = ptPuDptr->Blk;
        }
        else
        {
            //ptPuDptr->Blk = (ptPuDptr->Blk + 1) % BLK_PER_LUN;
            ptPuDptr->Blk = (ptPuDptr->Blk + L3_UTRand()) % BLK_PER_LUN;
        }
        usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk);
        if (INVALID_4F == usPhyBlk)
        {
            return L3_UT_LOCAL_STAGE_INIT;
        }

        if (TRUE == L2_VBT_Get_TLC(ucSuperPu, ptPuDptr->Blk))
        {
            g_ptPuMgDptr[ucTLun].FlashSLCMode = FALSE;
            g_ptPuMgDptr[ucTLun].LastPage = LOGIC_PG_PER_BLK - 1;
        }
        else
        {
            g_ptPuMgDptr[ucTLun].FlashSLCMode = TRUE;
#ifdef UTC_UN_CLOSED_BLK_WRITE_ENABLE
            g_ptPuMgDptr[ucTLun].LastPage = L3_UTRand() % PG_PER_SLC_BLK;
#else
            g_ptPuMgDptr[ucTLun].LastPage = PG_PER_SLC_BLK - 1;
#endif
        }

        ptPuDptr->Page = 0;
        L2_PBIT_Set_Free(ucSuperPu, ucLunInSuperPu, usPhyBlk, FALSE);
        DBG_Printf("MCU#%d SPU#%d LunInSPU#%d Blk#%d_%d INIT->ERS SLCMode=%d.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, usPhyBlk, g_ptPuMgDptr[ucTLun].FlashSLCMode);
        return L3_UT_LOCAL_STAGE_ERS;
    }

    // bad block, close the loop
    usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk);
    if (TRUE == L2_BbtIsGBbtBadBlock(ucTLun, usPhyBlk))
    {
        if (TRUE != L2_BM_BackUpBlockEmpty(ucSuperPu, ucLunInSuperPu))
        {
            U16 usNewPhyBlk;
            BOOL bTLCBlk = L2_PBIT_IsTLC(ucSuperPu, ucLunInSuperPu, usPhyBlk);

            usNewPhyBlk = L2_BM_AllocateBackUpBlock(ucSuperPu, ucLunInSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, bTLCBlk);
            L2_PBIT_Set_Backup(ucSuperPu, ucLunInSuperPu, usNewPhyBlk, FALSE);

            L2_PBIT_Set_Error(ucSuperPu, ucLunInSuperPu, usPhyBlk, TRUE);
            L2_PBIT_Set_Free(ucSuperPu, ucLunInSuperPu, usPhyBlk, FALSE);

            L2_PBIT_SetVirturlBlockAddr(ucSuperPu, ucLunInSuperPu, usNewPhyBlk, ptPuDptr->Blk);
            L2_VBT_SetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, usNewPhyBlk);

            // update the Block mode of the new phyblk.
            L2_PBIT_Set_TLC(ucSuperPu, ucLunInSuperPu, usNewPhyBlk, L2_VBT_Get_TLC(ucSuperPu, ptPuDptr->Blk));

            L2_Exchange_PBIT_Info(ucSuperPu, ucLunInSuperPu, usPhyBlk, usNewPhyBlk);
            L2_Reset_PBIT_Info(ucSuperPu, ucLunInSuperPu, usPhyBlk);

            ptPuDptr->Page = 0;
            DBG_Printf("MCU#%d SPU#%d LunInSPU#%d Blk#%d NewPhyBlk#%d Redo.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, usNewPhyBlk);
            return L3_UT_LOCAL_STAGE_ERS;
        }
        else
        {
            ptPuDptr->Page = ulPageNumPerBlkForTest;
            DBG_Printf("MCU#%d SPU#%d LunInSPU#%d Blk#%d BAD BLK->RD.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk);
            ucCurStage = L3_UT_LOCAL_STAGE_RD_4KSEQ;
        }
    }

    if (L3_UT_LOCAL_STAGE_END == ucCurStage)
    {
        DBG_Printf("MCU#%d [%d-%d,%d,%d] All Local Stage Test Passed %d Times.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ptPuDptr->BurnCnt);
        ucNextStage = L3_UT_LOCAL_STAGE_INIT;
    }

    else if (L3_UT_LOCAL_STAGE_ERS == ucCurStage)
    {
        L3_UTResetTotalPrgOrder(ucTLun);
        DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_ERS.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
        ucNextStage = L3_UT_LOCAL_STAGE_WT;
    }

    else if (L3_UT_LOCAL_STAGE_WT == ucCurStage)
    {
        ptPuDptr->Page++;
        if (ptPuDptr->Page < ulPageNumPerBlkForTest)
        {
            ucNextStage = L3_UT_LOCAL_STAGE_WT;
        }
        else
        {
            DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_WT Passed!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
            ptPuDptr->Page = 0;
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD : (L3_UT_LOCAL_STAGE_RD + L3_UTRand() % L3_UT_LOCAL_READ_NUM);
        }
    }

    else if (L3_UT_LOCAL_STAGE_RD == ucCurStage)
    {
        ptPuDptr->Page++;
        if (ptPuDptr->Page < ulPageNumPerBlkForTest)
        {
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD : (L3_UT_LOCAL_STAGE_RD + L3_UTRand() % L3_UT_LOCAL_READ_NUM);
        }
        else
        {
            DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD Passed!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
            ptPuDptr->Page = 0;
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_RED : L3_UT_LOCAL_STAGE_END;
        }
    }

    else if (L3_UT_LOCAL_STAGE_RD_RED == ucCurStage)
    {
        ptPuDptr->Page++;
        if (ptPuDptr->Page < ulPageNumPerBlkForTest)
        {
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_RED : (L3_UT_LOCAL_STAGE_RD + L3_UTRand() % L3_UT_LOCAL_READ_NUM);
        }
        else
        {
            DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_RED Passed!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
            ptPuDptr->Page = 0;
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_DATA : L3_UT_LOCAL_STAGE_END;
        }
    }

    else if (L3_UT_LOCAL_STAGE_RD_DATA == ucCurStage)
    {
        ptPuDptr->Page++;
        if (ptPuDptr->Page < ulPageNumPerBlkForTest)
        {
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_DATA : (L3_UT_LOCAL_STAGE_RD + L3_UTRand() % L3_UT_LOCAL_READ_NUM);
        }
        else
        {
            DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_DATA Passed!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
            ptPuDptr->Page = 0;
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_LPN : L3_UT_LOCAL_STAGE_END;
        }
    }

    else if (L3_UT_LOCAL_STAGE_RD_LPN == ucCurStage)
    {
        l_ucLPNCaseOffset = (l_ucLPNCaseOffset + 1) % l_ulMaxLpnRdCase;
        if (0 == l_ucLPNCaseOffset % (l_ulMaxLpnRdCase >> LPN_PER_BUF_BITS))
        {
            ptPuDptr->Page += L3_UTRand() % 10;// for reduce the read page times.
        }

        if (ptPuDptr->Page < ulPageNumPerBlkForTest)
        {
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_LPN : (L3_UT_LOCAL_STAGE_RD + L3_UTRand() % L3_UT_LOCAL_READ_NUM);
        }
        else
        {
            DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_LPN Passed!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
            ptPuDptr->Page = 0;
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_MERGE : L3_UT_LOCAL_STAGE_END;
        }
    }

    else if (L3_UT_LOCAL_STAGE_RD_MERGE == ucCurStage)
    {
        ptPuDptr->Page++;
        if (ptPuDptr->Page < ulPageNumPerBlkForTest)
        {
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_MERGE : (L3_UT_LOCAL_STAGE_RD + L3_UTRand() % L3_UT_LOCAL_READ_NUM);
        }
        else
        {
            DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_MERGE Passed!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
            ptPuDptr->Page = 0;
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_4KSEQ : L3_UT_LOCAL_STAGE_END;
        }
    }

    else if (L3_UT_LOCAL_STAGE_RD_4KSEQ == ucCurStage)
    {
        LOCAL U32 s_ul4KSeqReadCycleCnt = 0;

        ptPuDptr->Page++;
        if (ptPuDptr->Page < ulPageNumPerBlkForTest)
        {
            ucNextStage = (TRUE != bRandomReadEn) ? L3_UT_LOCAL_STAGE_RD_4KSEQ : (L3_UT_LOCAL_STAGE_RD + L3_UTRand() % L3_UT_LOCAL_READ_NUM);
        }
        else
        {
            s_ul4KSeqReadCycleCnt++;
            DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_4KSEQ Passed! CycleCnt=0x%x\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, s_ul4KSeqReadCycleCnt);
            ptPuDptr->Page = 0;
            ucNextStage = L3_UT_LOCAL_STAGE_END;
        }
    }

    else
    {
        DBG_Printf("MCU#%d L3_UTSelectNextStage Stage Error %d\n", HAL_GetMcuId(), ucCurStage);
        DBG_Getch();
    }

    return ucNextStage;
}

void L3_UTRedGen(RED *pSpareAddr, BOOL bSinglePln)
{
    U8 ucTmp = 0, ucPlnNum;
    U32 DwIndex, TargetAddr;

    TargetAddr = (U32)pSpareAddr;
    ucPlnNum = (FALSE == bSinglePln) ? PLN_PER_LUN : 1;

    for (ucTmp = 0; ucTmp < ucPlnNum; ucTmp++)
    {
        for (DwIndex = 0; DwIndex < 4; DwIndex++)
        {
            COM_MemSet((U32*)(TargetAddr + ucTmp*RED_SZ + DwIndex*sizeof(U32)), 1, L3_UT_DUMMY_RED(0, 0, DwIndex + ucTmp * 4));
        }
    }

    return;
}

void L3_UTPrepareForWriteFor3DTlc(U8 ucSuperPu, U8 ucLunInSuperPu, PhysicalAddr *phyAddr, L3_UT_PU_MG_DPTR * ptPuDptr)
{
    U32 ulTLun;
    U32 ulSecIndex;
    U32 ulLPNIndex;
    U32 ulDummyData;
    //U32 ulPhyPgNum;
    //U32 ulSecNumPerPageForTest;
    RED *pRed = NULL;
    U8 ucTLCPairPageType;
    //U16 usUpperPage;
    U8 ucPartIndex, ucBuffCnt;
    //U8 ucPageIndex;
    static U32 aBuffAddr[DSG_BUFF_SIZE] = { 0 };
    static U32 aRedAddr[DSG_BUFF_SIZE] = { 0 };
    U16 aPageNum[DSG_BUFF_SIZE] = { 0 };

    /*convert [ucSuperPu:ucLunInSuperPu] to [ucPu:ucBLun]*/
    ulTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

    phyAddr->m_PUSer = ucSuperPu;
    phyAddr->m_OffsetInSuperPage = ucLunInSuperPu;
    phyAddr->m_BlockInPU = ptPuDptr->Blk;
    phyAddr->m_PageInBlock = ptPuDptr->Page;
    phyAddr->m_LPNInPage = 0;

    // allocate flash status and check it before send the next program command.
    ptPuDptr->FlashStatusBuffAddr = L3_UTGetStatusBuffAddr(ulTLun);

    // generate the red address and data buffer address for the program command
    ucTLCPairPageType = HAL_GetFlashPairPageType(ptPuDptr->Page);

    if (LOW_PAGE_WITHOUT_HIGH == ucTLCPairPageType)
    {
        ucBuffCnt = 1;

        aPageNum[0] = ptPuDptr->Page;
        aBuffAddr[0] = L3_UTGetDataBuffAddrFor2DTlcWrite(ulTLun, s_ulTotalPrgOrder[ulTLun], 0);
        COM_MemZero((U32 *)aBuffAddr[0], LOGIC_PIPE_PG_SZ / sizeof(U32));
    }
    else if (LOW_PAGE == ucTLCPairPageType)
    {
        ucBuffCnt = 2;

        aPageNum[0] = ptPuDptr->Page;
        aBuffAddr[0] = L3_UTGetDataBuffAddrFor2DTlcWrite(ulTLun, s_ulTotalPrgOrder[ulTLun], 0);
        COM_MemZero((U32 *)aBuffAddr[0], LOGIC_PIPE_PG_SZ / sizeof(U32));

        aPageNum[1] = ptPuDptr->Page + 1;
        aBuffAddr[1] = L3_UTGetDataBuffAddrFor2DTlcWrite(ulTLun, s_ulTotalPrgOrder[ulTLun], 1);
        COM_MemZero((U32 *)aBuffAddr[1], LOGIC_PIPE_PG_SZ / sizeof(U32));

        ptPuDptr->Page++; // skip the upper page
    }
    else if (EXTRA_PAGE == ucTLCPairPageType)
    {
        ucBuffCnt = 2;

        aPageNum[0] = ptPuDptr->Page;
        aBuffAddr[0] = L3_UTGetDataBuffAddrFor2DTlcWrite(ulTLun, s_ulTotalPrgOrder[ulTLun], 0);
        COM_MemZero((U32 *)aBuffAddr[0], LOGIC_PIPE_PG_SZ / sizeof(U32));
#ifdef FLASH_IM_3DTLC_GEN2
        aPageNum[1] = ptPuDptr->Page + 1;
        ptPuDptr->Page++; // skip the upper of extra page
#else
        aPageNum[1] = HAL_GetHighPageIndexfromExtra(ptPuDptr->Page);
#endif
        aBuffAddr[1] = L3_UTGetDataBuffAddrFor2DTlcWrite(ulTLun, s_ulTotalPrgOrder[ulTLun], 1);
        COM_MemZero((U32 *)aBuffAddr[1], LOGIC_PIPE_PG_SZ / sizeof(U32));
    }
    else
    {
        DBG_Getch();
    }


    aRedAddr[0] = L3_UTGetSpareBuffAddrFor2DTlcWrite(ulTLun, s_ulTotalPrgOrder[ulTLun], 0);
    COM_MemZero((U32 *)aRedAddr[0], sizeof(RED)*ucBuffCnt / sizeof(U32));

    ptPuDptr->SpareBuffAddr = (U32)aRedAddr[0];
    ptPuDptr->DataBuffAddr = (U32)aBuffAddr;

#if 0
    DBG_Printf("TLUN%d Blk%d PrgPage%d FlashStatus=0x%x, RedAddr=0x%x, DataBuff=0x%x,0x%x,0x%x, BufID=0x%x,0x%x,0x%x\n",
        ulTLun, ptPuDptr->Blk, ptPuDptr->Page,
        ptPuDptr->FlashStatusBuffAddr,
        ptPuDptr->SpareBuffAddr,
        *((U32*)ptPuDptr->DataBuffAddr),
        *((U32*)ptPuDptr->DataBuffAddr + 1),
        *((U32*)ptPuDptr->DataBuffAddr + 2),
        COM_GetBufferIDByMemAddr(*((U32*)ptPuDptr->DataBuffAddr), TRUE, LOGIC_PIPE_PG_SZ_BITS),
        COM_GetBufferIDByMemAddr(*((U32*)ptPuDptr->DataBuffAddr + 1), TRUE, LOGIC_PIPE_PG_SZ_BITS),
        COM_GetBufferIDByMemAddr(*((U32*)ptPuDptr->DataBuffAddr + 2), TRUE, LOGIC_PIPE_PG_SZ_BITS));
#endif

    // prepare data and red.
    for (ucPartIndex = 0; ucPartIndex < ucBuffCnt; ucPartIndex++)
    {
        U32 ulDataBaseAddr, ulRedBaseAddr;
        U16 usPageNum;

        ulDataBaseAddr = aBuffAddr[ucPartIndex];
        ulRedBaseAddr = aRedAddr[0] + (RED_SZ * PLN_PER_LUN * ucPartIndex);
        usPageNum = aPageNum[ucPartIndex];

        // init red area & data area.
        ulDummyData = UT_GET_DUMMY_DATA(ulTLun, ptPuDptr->Blk, usPageNum, 1);
        pRed = (RED *)ulRedBaseAddr;
        pRed->m_RedComm.bcPageType = PAGE_TYPE_DATA;
        pRed->m_RedComm.ulTimeStamp = ulDummyData;
        pRed->m_RedComm.ulTargetOffsetTS = ulDummyData;

        ulLPNIndex = 0;
        for (ulSecIndex = 0; ulSecIndex < SEC_PER_BUF; ulSecIndex++)
        {
            // Note: Inorder to meet mode data check solution, we should
            // init data in the second dw each sec, and set Red Lpn data
            // according ulDummyData. -- with nina's advice.
            ulDummyData = UT_GET_DUMMY_DATA(ulTLun, ptPuDptr->Blk, usPageNum, ulSecIndex);
            *((volatile U32 *)(ulDataBaseAddr + ulSecIndex*SEC_SIZE) + 1) = ulDummyData;

            if (0 == ulSecIndex%SEC_PER_LPN)
            {
                pRed->m_DataRed.aCurrLPN[ulLPNIndex++] = L0M_GET_SUBSYSLBA_FROM_LBA(ulDummyData, l_ulSubsysNumBits) / SEC_PER_LPN;
            }
        }

        #ifdef EM_LBA_CHKEN
        L3_UTRedGen(pRed, ptPuDptr->FlashSingePln);
        #endif
    }

    s_ulTotalPrgOrder[ulTLun]++;

    return;
}

BOOL L3_UTPrepareForWriteFor2DTlc(U8 ucSuperPu, U8 ucLunInSuperPu, PhysicalAddr *phyAddr, L3_UT_PU_MG_DPTR * ptPuDptr)
{
    U32 ulTLun;
    U32 ulSecIndex;
    U32 ulLPNIndex;
    U32 ulDummyData;
    U32 ulPhyPgNum;
    U32 ulSecNumPerPageForTest;
    RED *pRed = NULL;

    U8 ucPartIndex;
    U16 usPrgWL;
    U8 ucPrgCycle;
    U8 ucPageIndex;
    static U32 aBuffAddr[SUBSYSTEM_LUN_MAX][DSG_BUFF_SIZE*PG_PER_WL] = { 0 };
    static U32 aRedAddr[SUBSYSTEM_LUN_MAX][DSG_BUFF_SIZE*PG_PER_WL] = { 0 };
    static U32 aStatusAddr[SUBSYSTEM_LUN_MAX][DSG_BUFF_SIZE][PG_PER_WL] = { 0 };

    /*convert [ucSuperPu:ucLunInSuperPu] to [ucPu:ucBLun]*/
    ulTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

    phyAddr->m_PUSer = ucSuperPu;
    phyAddr->m_OffsetInSuperPage = ucLunInSuperPu;
    phyAddr->m_BlockInPU = ptPuDptr->Blk;
    phyAddr->m_PageInBlock = ptPuDptr->Page;
    phyAddr->m_LPNInPage = 0;

    // calculate the WL and PrgCycle
    usPrgWL = HAL_FlashGetTlcPrgWL(ptPuDptr->Page);
    ucPrgCycle = HAL_FlashGetTlcPrgCycle(ptPuDptr->Page);

    // allocate flash status and check it before send the next program command.
    ptPuDptr->FlashStatusBuffAddr = L3_UTGetStatusBuffAddr(ulTLun);
    if (aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][ucPrgCycle] == 0)
    {
        aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][ucPrgCycle] = ptPuDptr->FlashStatusBuffAddr;
    }
    else
    {
        if (*(U8*)aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][2] == SUBSYSTEM_STATUS_PENDING)
        {
            return FALSE;
        }
        else
        {
            aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][ucPrgCycle] = ptPuDptr->FlashStatusBuffAddr;
        }
    }

    // generate the red address and data buffer address for the program command
    if (0 == ucPrgCycle)
    {
        for (ucPageIndex = 0; ucPageIndex < DSG_BUFF_SIZE; ucPageIndex++)
        {
            aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex] = L3_UTGetSpareBuffAddrFor2DTlcWrite(ulTLun, usPrgWL, ucPageIndex);
            COM_MemZero((U32 *)aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex], sizeof(RED) / sizeof(U32));

            aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex] = L3_UTGetDataBuffAddrFor2DTlcWrite(ulTLun, usPrgWL, ucPageIndex);
            COM_MemZero((U32 *)aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex], LOGIC_PIPE_PG_SZ / sizeof(U32));
        }
    }
    ptPuDptr->SpareBuffAddr = (U32)aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE];
    ptPuDptr->DataBuffAddr = (U32)&aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE];

#if 0
    DBG_Printf("TLUN%d Blk%d PrgTime%d WL%d Cycle%d FlashStatus=0x%x, RedAddr=0x%x, DataBuff=0x%x,0x%x,0x%x\n",
        ulTLun, ptPuDptr->Blk, ptPuDptr->Page, usPrgWL, ucPrgCycle,
        ptPuDptr->FlashStatusBuffAddr,
        ptPuDptr->SpareBuffAddr,
        *((U32*)ptPuDptr->DataBuffAddr),
        *((U32*)ptPuDptr->DataBuffAddr+1),
        *((U32*)ptPuDptr->DataBuffAddr+2));
#endif

    // if it is not the first program cycle, no need to init the red&data buffer.
    if (0 != ucPrgCycle)
    {
        return TRUE;
    }

    ulPhyPgNum = usPrgWL*PG_PER_WL;
    ulSecNumPerPageForTest = (FALSE == ptPuDptr->FlashSingePln) ? SEC_PER_BUF : SEC_PER_LOGIC_PG;

    for (ucPartIndex = 0; ucPartIndex < DSG_BUFF_SIZE; ucPartIndex++)
    {
        U32 ulDataBaseAddr, ulRedBaseAddr;

        ulDataBaseAddr = aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPartIndex];
        ulRedBaseAddr = aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPartIndex];

        // init red area & data area.
        ulDummyData = UT_GET_DUMMY_DATA(ulTLun, ptPuDptr->Blk, ulPhyPgNum + ucPartIndex, 1);
        pRed = (RED *)ulRedBaseAddr;
        pRed->m_RedComm.bcPageType = PAGE_TYPE_DATA;
        pRed->m_RedComm.ulTimeStamp = ulDummyData;
        pRed->m_RedComm.ulTargetOffsetTS = ulDummyData;

        ulLPNIndex = 0;

        for (ulSecIndex = 0; ulSecIndex < ulSecNumPerPageForTest; ulSecIndex++)
        {
            // Note: Inorder to meet mode data check solution, we should
            // init data in the second dw each sec, and set Red Lpn data
            // according ulDummyData. -- with nina's advice.
            ulDummyData = UT_GET_DUMMY_DATA(ulTLun, ptPuDptr->Blk, ulPhyPgNum + ucPartIndex, ulSecIndex);
            *((volatile U32 *)(ulDataBaseAddr + ulSecIndex*SEC_SIZE) + 1) = ulDummyData;

            if (0 == ulSecIndex%SEC_PER_LPN)
            {
                pRed->m_DataRed.aCurrLPN[ulLPNIndex++] = L0M_GET_SUBSYSLBA_FROM_LBA(ulDummyData, l_ulSubsysNumBits) / SEC_PER_LPN;
            }
        }

#ifdef EM_LBA_CHKEN
        L3_UTRedGen(pRed, ptPuDptr->FlashSingePln);
#endif
    }

    return TRUE;
}


void L3_UTPrepareForWrite(U8 ucSuperPu, U8 ucLunInSuperPu, PhysicalAddr *phyAddr, L3_UT_PU_MG_DPTR * ptPuDptr)
{
    U32 ulTLun;
    U32 ulSecIndex;
    U32 ulLPNIndex;
    U32 ulDummyData;
    U32 ulPhyPgNum;
    U32 ulSecNumPerPageForTest;
    RED *pRed = NULL;

    /*convert [ucSuperPu:ucLunInSuperPu] to [ucPu:ucBLun]*/
    ulTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

    phyAddr->m_PUSer = ucSuperPu;
    phyAddr->m_OffsetInSuperPage = ucLunInSuperPu;
    phyAddr->m_BlockInPU = ptPuDptr->Blk;
    phyAddr->m_PageInBlock = ptPuDptr->Page;
    phyAddr->m_LPNInPage = 0;

    ptPuDptr->FlashStatusBuffAddr = L3_UTGetStatusBuffAddr(ulTLun);

    ptPuDptr->DataBuffAddr = L3_UTGetDataBuffAddr(ulTLun);
    COM_MemZero((U32 *)ptPuDptr->DataBuffAddr, LOGIC_PIPE_PG_SZ / sizeof(U32));

    ptPuDptr->SpareBuffAddr = L3_UTGetSpareBuffAddr(ulTLun);
    COM_MemZero((U32 *)ptPuDptr->SpareBuffAddr, sizeof(RED) / sizeof(U32));

    ulPhyPgNum = ptPuDptr->Page;
    ulSecNumPerPageForTest = (FALSE == ptPuDptr->FlashSingePln) ? SEC_PER_BUF : SEC_PER_LOGIC_PG;

    // init red area & data area.
    ulDummyData = UT_GET_DUMMY_DATA(ulTLun, ptPuDptr->Blk, ulPhyPgNum, 1);
    *(volatile U32 *)(ptPuDptr->SpareBuffAddr) = ulDummyData;
    pRed = (RED *)ptPuDptr->SpareBuffAddr;
    pRed->m_RedComm.bcPageType = PAGE_TYPE_DATA;
    pRed->m_RedComm.ulTimeStamp = ulDummyData;
    pRed->m_RedComm.ulTargetOffsetTS = ulDummyData;

    ulLPNIndex = 0;
    for (ulSecIndex = 0; ulSecIndex < ulSecNumPerPageForTest; ulSecIndex++)
    {
        // Note: Inorder to meet mode data check solution, we should
        // init data in the second dw each sec, and set Red Lpn data
        // according ulDummyData. -- with nina's advice.
        ulDummyData = UT_GET_DUMMY_DATA(ulTLun, ptPuDptr->Blk, ulPhyPgNum, ulSecIndex);
        *((volatile U32 *)(ptPuDptr->DataBuffAddr + ulSecIndex*SEC_SIZE) + 1) = ulDummyData;

        if (0 == ulSecIndex%SEC_PER_LPN)
        {
            pRed->m_DataRed.aCurrLPN[ulLPNIndex++] = L0M_GET_SUBSYSLBA_FROM_LBA(ulDummyData, l_ulSubsysNumBits) / SEC_PER_LPN;
        }
    }

#ifdef EM_LBA_CHKEN
    L3_UTRedGen(pRed, ptPuDptr->FlashSingePln);
#endif

    return;
}

void L3_UTPrepareForRD(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucCurStage, PhysicalAddr *phyAddr, L3_UT_PU_MG_DPTR * ptPuDptr, BUF_REQ_READ *pBufReq )
{
    U8 ucTLun;
    U8 ucLpnIndex;

    /*convert [ucSuperPu:ucLunInSuperPu] to [ucPu:ucBLun]*/
    ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

    if ((L3_UT_LOCAL_STAGE_RD == ucCurStage)
    || (L3_UT_LOCAL_STAGE_RD_RED == ucCurStage)
    || (L3_UT_LOCAL_STAGE_RD_DATA== ucCurStage)
    || (L3_UT_LOCAL_STAGE_RD_LPN== ucCurStage))
    {
        phyAddr->m_PUSer = ucSuperPu;
        phyAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        phyAddr->m_BlockInPU = ptPuDptr->Blk;
        phyAddr->m_PageInBlock = ptPuDptr->Page;
        phyAddr->m_LPNInPage = 0;

        ptPuDptr->FlashStatusBuffAddr = L3_UTGetStatusBuffAddr(ucTLun);

        ptPuDptr->DataBuffAddr = L3_UTGetDataBuffAddr(ucTLun);
        COM_MemZero((U32 *)ptPuDptr->DataBuffAddr, LOGIC_PIPE_PG_SZ / sizeof(U32));

        ptPuDptr->SpareBuffAddr = L3_UTGetSpareBuffAddr(ucTLun);
        COM_MemZero((U32 *)ptPuDptr->SpareBuffAddr, sizeof(RED) / sizeof(U32));

        if (L3_UT_LOCAL_STAGE_RD_LPN== ucCurStage)
        {
            phyAddr->m_LPNInPage = l_aUTLpnRdCase[l_ucLPNCaseOffset].StartInPage;
        }
    }
    else if (L3_UT_LOCAL_STAGE_RD_MERGE== ucCurStage)
    {
        ptPuDptr->DataBuffAddr = L3_UTGetDataBuffAddr(ucTLun);
        COM_MemZero((U32 *)ptPuDptr->DataBuffAddr, LOGIC_PIPE_PG_SZ / sizeof(U32));

        // init buf-req.
        COM_MemZero((U32*)pBufReq, sizeof(BUF_REQ_READ) / sizeof(U32));
        pBufReq->bReadPreFetch = TRUE;
        pBufReq->usPhyBufferID = COM_GetBufferIDByMemAddr(ptPuDptr->DataBuffAddr, TRUE, BUF_SIZE_BITS);
        pBufReq->ulReqStatusAddr = L3_UTGetStatusBuffAddrOTFB(ucTLun);
        *(U8 *)pBufReq->ulReqStatusAddr = SUBSYSTEM_STATUS_PENDING;

        pBufReq->ucLPNOffset = L3_UTRand() % LPN_PER_BUF;
        pBufReq->ulLPNBitMap = 1 + L3_UTRand() % MULTI_VALUE_1(SEC_PER_LPN);

        for (ucLpnIndex = 0; ucLpnIndex < LPN_PER_BUF; ucLpnIndex++)
        {
            phyAddr[ucLpnIndex].m_PUSer = ucSuperPu;
            phyAddr[ucLpnIndex].m_OffsetInSuperPage = ucLunInSuperPu;
            phyAddr[ucLpnIndex].m_BlockInPU = ptPuDptr->Blk;
            phyAddr[ucLpnIndex].m_PageInBlock = ptPuDptr->Page;
            phyAddr[ucLpnIndex].m_LPNInPage = ucLpnIndex;
        }
    }
    else
    {
        DBG_Getch();
    }

    return;
}

void L3_UTPrepareFor4KSEQR(U8 ucSuperPu, U8 ucLunInSuperPu, L3_UT_PU_MG_DPTR * ptPuDptr, FCMD_REQ_ITEM *ptFCmdReqItem, U16 *pBuffID, FCMD_READ_RANGE *ptRange)
{
    U8 ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

    ptFCmdReqItem->bsTLun = ucTLun;
    ptFCmdReqItem->bsPln = 0;
    ptFCmdReqItem->bsBlk = ptPuDptr->Blk;
    ptFCmdReqItem->bsPage = ptPuDptr->Page;
    ptFCmdReqItem->bsFCmdType = FCMD_REQ_TYPE_READ;
    ptFCmdReqItem->bsFCmdSubType = FCMD_REQ_SUBTYPE_NORMAL;
    ptFCmdReqItem->bsTableReq = FALSE;
    ptFCmdReqItem->bsSLCMode = ptPuDptr->FlashSLCMode;
    ptFCmdReqItem->bsShiftReadEn = FALSE;

    ptRange->bsLPNInBuff = 0;
    ptRange->bsLPNInPage = L3_UTRand()%LPN_PER_BUF;
    ptRange->bsLPNCnt = 1;

    ptPuDptr->DataBuffAddr = L3_UTGetDataBuffAddr(ucTLun);
    //COM_MemZero((U32 *)ptPuDptr->DataBuffAddr, LOGIC_PIPE_PG_SZ / sizeof(U32));

    pBuffID[0] = COM_GetBufferIDByMemAddr(ptPuDptr->DataBuffAddr, TRUE, BUF_SIZE_BITS);

    ptPuDptr->FlashStatusBuffAddr = L3_UTGetStatusBuffAddrOTFB(ucTLun);

    return;
}

/*==============================================================================
Func Name  : L3_UTDriverNoramlTest
Input      : None
Output     : None
Return Val :
Discription: Flash Request Pattern.
Usage      : Erase -> Program (Closed Blk or Open Blk) -> MixRead -> DataCheck
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_UTDriverNoramlTest(void)
{
    U8 ucSuperPu;
    U8 ucLunInSuperPu;
    U8 ucTLun;
    U8 ucNextStage;
    BUF_REQ_READ tReadBufReq;
    L3_UT_PU_MG_DPTR *ptPuDptr;
    PhysicalAddr   phyAddr = { 0 };
    PhysicalAddr atAddr[LPN_PER_BUF] = { 0 };
    BOOL bFinishNormalTest = FALSE;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        /*select one LunInSuperPu to test*/
        ucLunInSuperPu = L3_UTSelectLunForPush(ucSuperPu);
        if (INVALID_2F == ucLunInSuperPu)
        {
            continue;
        }

        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (FALSE == L2_FCMDQNotFull(ucTLun))
        {
            DBG_Getch();
        }

        ptPuDptr = &g_ptPuMgDptr[ucTLun];

        switch (ptPuDptr->CurStage)
        {
            case L3_UT_LOCAL_STAGE_INIT:
            {
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_INIT);
                break;
            }
            case L3_UT_LOCAL_STAGE_ERS:
            {
                L3_UTDataCheck(ucTLun);

                L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, NULL, FALSE, ptPuDptr->FlashSLCMode,TRUE);
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_ERS);
                break;
            }
            case L3_UT_LOCAL_STAGE_WT:
            {
                L3_UTDataCheck(ucTLun);

                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_WT SLC=%d\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ptPuDptr->FlashSLCMode);

                if (TRUE == ptPuDptr->FlashSLCMode)
                {
                    L3_UTPrepareForWrite(ucSuperPu, ucLunInSuperPu, &phyAddr, ptPuDptr);
                    //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_WT SLC=%d\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ptPuDptr->FlashSLCMode);
                    L2_FtlWriteLocal(&phyAddr, (U32 *)ptPuDptr->DataBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, FALSE, ptPuDptr->FlashSLCMode, NULL);
                }
                else
                {
                    #ifdef FLASH_INTEL_3DTLC
                    L3_UTPrepareForWriteFor3DTlc(ucSuperPu, ucLunInSuperPu, &phyAddr, ptPuDptr);
                    #else
                    if (FALSE == L3_UTPrepareForWriteFor2DTlc(ucSuperPu, ucLunInSuperPu, &phyAddr, ptPuDptr))
                    {
                        ucNextStage = L3_UT_LOCAL_STAGE_WT;
                        break;
                    }
                    #endif

                    //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_WT SLC=%d\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page,ptPuDptr->FlashSLCMode);
                    L2_FtlTLCExternalWriteLocal(&phyAddr, (U32 *)ptPuDptr->DataBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, NULL);
                }

                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_WT);
                break;
            }
            case L3_UT_LOCAL_STAGE_RD:
            {
                L3_UTDataCheck(ucTLun);

                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD SLC=%d.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ptPuDptr->FlashSLCMode);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, LPN_PER_BUF, 0, FALSE, ptPuDptr->FlashSLCMode);
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD);
                break;
            }
            case L3_UT_LOCAL_STAGE_RD_RED:
            {
                L3_UTDataCheck(ucTLun);

                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_RED, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_RED.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, 0, 0, FALSE, ptPuDptr->FlashSLCMode);
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_RED);
                break;
            }
            case L3_UT_LOCAL_STAGE_RD_DATA:
            {
                L3_UTDataCheck(ucTLun);

                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_DATA, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_DATA.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, NULL, LPN_PER_BUF, 0, FALSE, ptPuDptr->FlashSLCMode);
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_DATA);
                break;
            }
            case L3_UT_LOCAL_STAGE_RD_LPN:
            {
                L3_UTDataCheck(ucTLun);

                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_LPN, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_LPN [%d, %d, %d, %d].\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, l_ucLPNCaseOffset, phyAddr.m_LPNInPage, l_aUTLpnRdCase[l_ucLPNCaseOffset].StartInBuf, l_aUTLpnRdCase[l_ucLPNCaseOffset].Cnt);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, NULL, l_aUTLpnRdCase[l_ucLPNCaseOffset].Cnt, l_aUTLpnRdCase[l_ucLPNCaseOffset].StartInBuf, FALSE, ptPuDptr->FlashSLCMode);
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_LPN);
                break;
            }
            case L3_UT_LOCAL_STAGE_RD_MERGE:
            {
                L3_UTDataCheck(ucTLun);

                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_MERGE, atAddr, ptPuDptr, &tReadBufReq);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_NORMAL_STAGE_MERGE_RD.BufID=0x%x LpnOffSet=0x%x, LpnBitMap=0x%x\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, tBufReq.PhyBufferID, tBufReq.LPNOffset, tBufReq.LPNBitMap);
                if (TRUE != L2_FtlReadMerge(ucSuperPu, INVALID_8F, &tReadBufReq, atAddr))
                {
                    DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_MERGE getch.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                    DBG_Getch();
                }

                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_MERGE);
                break;
            }
            case L3_UT_LOCAL_STAGE_RD_4KSEQ:
            {
                U16 aBuffId[DSG_BUFF_SIZE] = { 0 };
                FCMD_REQ_ITEM tFCmdReqItem = { 0 };
                FCMD_READ_RANGE tRange;

                L3_UTDataCheck(ucTLun);

                L3_UTPrepareFor4KSEQR(ucSuperPu, ucLunInSuperPu, ptPuDptr, &tFCmdReqItem, aBuffId, &tRange);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_4KSEQ LPNCnt=%d.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, tRange.bsLPNCnt);
                L3_NormalReadPage(&tFCmdReqItem, aBuffId, &tRange, (U8*)ptPuDptr->FlashStatusBuffAddr);

                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_4KSEQ);

                break;
            }
            case L3_UT_LOCAL_STAGE_END:
            {
                if (ptPuDptr->BurnCnt > FCMD_TEST_BURN_CNT)
                {
                    bFinishNormalTest = TRUE;
                }
                ptPuDptr->BurnCnt++;

                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_END);

                break;
            }
            default:
            {
                DBG_Printf("MCU#%d [%d-%d,%d,%d] Error Local Stage=0x%x!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ptPuDptr->CurStage);
                DBG_Getch();
            }
        }

        ptPuDptr->CurStage = ucNextStage;
    }

    return bFinishNormalTest;
}

GLOBAL void MCU1_DRAM_TEXT UT_MemInit(void)
{
    L3_UTDramVarInit(&g_FreeMemBase.ulDRAMBase);
    L3_UTOtfbVarInit(&g_FreeMemBase.ulFreeCacheStatusBase);
}

/*==============================================================================
Func Name  : L3_UTBasicInit
Input      : None
Output     : None
Return Val :
Discription: L3 Unit Test Init.
Usage      :
History    :
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
void MCU1_DRAM_TEXT L3_UTBasicInit(void)
{
    LOCAL BOOL bUTInitDone = FALSE;

    // the other l3 unit test case can use this interface to init the basic mem.
    if (FALSE == bUTInitDone)
    {
        //UT_MemInit();

        L3_UTSrand(0xA5A5A5A5);

        L3_UTLpnReadCaseInit();

        L3_UTPartialReadCaseInit();

        l_ulSubsysNumBits = (1 < DiskConfig_GetSubSysNum()) ? 1 : 0;

        bUTInitDone = TRUE;
    }

    L3_UTMgDptrInit();

    L3_UTInitFCmdPtrForDataCheck();

    return;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_FCmdTest
Input      : void
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2014.12.19 JasonGuo create function
==============================================================================*/
BOOL L3_FCmdTest(void)
{
    BOOL bFCmdTestDone = FALSE;
    LOCAL U32 l_ulL3UTDriverStatus = FCMD_TEST_STATE_INIT;

    switch (l_ulL3UTDriverStatus)
    {
        case FCMD_TEST_STATE_INIT:
        {
            L3_UTBasicInit();
            l_ulL3UTDriverStatus = FCMD_TEST_STATE_RUN;
            break;
        }
        case FCMD_TEST_STATE_RUN:
        {
            if (TRUE == L3_UTDriverNoramlTest())
            {
                l_ulL3UTDriverStatus = FCMD_TEST_STATE_SAVEBBT;
            }
            break;
        }
        case FCMD_TEST_STATE_SAVEBBT:
        {
            if (TRUE == L2_BbtIsSavedDone())
            {
                l_ulL3UTDriverStatus = FCMD_TEST_STATE_DONE;
                DBG_Printf("Bbt Save Done.\n");
            }
            else
            {
                L2_BbtSchedule();
            }
            break;
        }
        case FCMD_TEST_STATE_DONE:
        {
            DBG_Printf("\nMCU#%d L3_FCmdTest Finish.\n\n", HAL_GetMcuId());
            l_ulL3UTDriverStatus = FCMD_TEST_STATE_INIT;
            bFCmdTestDone = TRUE;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d L3_FCmdTest Status=%d Err.\n", HAL_GetMcuId(), l_ulL3UTDriverStatus);
            DBG_Getch();
        }
    }

    return bFCmdTestDone;
}

GLOBAL void UT_Wait(U32 ulBase)
{
    U32 ulCnt = L3_UTRand() % ulBase;
    U32 ulDelayCalc;

    while (ulCnt != 0)
    {
        ulDelayCalc = 100 * 200 / 23;
        ulCnt--;
    }

    return;
}

/*====================End of this file========================================*/

