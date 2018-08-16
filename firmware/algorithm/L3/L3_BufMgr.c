/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
********************************************************************************
* File Name    : L3_BufMgr.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "FW_BufAddr.h"
#include "HAL_Xtensa.h"
#include "L3_BufMgr.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
/********************************************/
/* APP1: Dram Buffer for BlkSwap Management */
/********************************************/
LOCAL SDL_MGR l_tSDLBufMgr;
LOCAL U32 l_ulSDLBufMgrBase;
LOCAL U32 l_ulSDLBufMemBase;

/*******************************************/
/* APP2: Red Resource Management           */
/*******************************************/
LOCAL SDL_MGR l_tSDLRedMgr;
LOCAL U32 l_ulSDLRedMgrBase;
LOCAL U32 l_ulSDLRedMemBase;

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_MgrAllcSRAM0
Input      : U32 *pFreeSram0Base
Output     : NONE
Return Val : void
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_MgrAllcSRAM0(U32 *pFreeSram0Base)
{
    U32 ulFreeBase = *pFreeSram0Base;
    COM_MemAddr16DWAlign(&ulFreeBase);

    l_ulSDLBufMgrBase = ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(SDL_HEAD) + SDL_BUF_NUM * sizeof(SDL_NODE));
    COM_MemAddr16DWAlign(&ulFreeBase);
    DBG_Printf("L3: 1MgrAllcSRAM0 %dKB, -> %dKB -> totalSRAM0 %dKB\n", (ulFreeBase - *pFreeSram0Base) / 1024, (ulFreeBase - DSRAM0_MCU2_BASE) / 1024, DSRAM0_MCU2_MAX_SIZE / 1024);

    l_ulSDLRedMgrBase = ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(SDL_HEAD) + SDL_RED_NUM * sizeof(SDL_NODE));
    COM_MemAddr16DWAlign(&ulFreeBase);

    DBG_Printf("L3: 2MgrAllcSRAM0 %dKB, -> %dKB -> totalSRAM0 %dKB\n", (ulFreeBase - *pFreeSram0Base) / 1024, (ulFreeBase - DSRAM0_MCU2_BASE)/1024, DSRAM0_MCU2_MAX_SIZE / 1024);
    ASSERT(ulFreeBase - DSRAM0_MCU2_BASE < DSRAM0_MCU2_MAX_SIZE);

    *pFreeSram0Base = ulFreeBase;

    return;
}

/*==============================================================================
Func Name  : L3_MgrAllcDram
Input      : U32 *pFreeDramBase
Output     : NONE
Return Val : void
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_MgrAllcDram(U32 *pFreeDramBase)
{
    U32 ulFreeBase = *pFreeDramBase;

    COM_MemAddr16DWAlign(&ulFreeBase);
    l_ulSDLRedMemBase = ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, SDL_RED_NUM * SDL_RED_SIZE);

    COM_MemAddrPageBoundaryAlign(&ulFreeBase);
    l_ulSDLBufMemBase = ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, SDL_BUF_NUM * SDL_BUF_SIZE);

    ASSERT(ulFreeBase-DRAM_DATA_BUFF_MCU2_BASE < DATA_BUFF_MCU2_SIZE);
    *pFreeDramBase = ulFreeBase;

    return;
}

/*==============================================================================
Func Name  : L3_BufMgrInit
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_BufMgrInit(void)
{
    SDL_SetListMem(&l_tSDLBufMgr, l_ulSDLBufMgrBase, l_ulSDLBufMemBase);
    SDL_Init(&l_tSDLBufMgr, SDL_BUF_NUM, SDL_BUF_SIZE);

    SDL_ASSERT(l_tSDLBufMgr.ptHead->bsFreeCnt + l_tSDLBufMgr.ptHead->bsBsyCnt == SDL_BUF_NUM);
    //SDL_ASSERT(SDL_ShowList(&l_tSDLBufMgr, SDL_NODE_STS_FREE, TRUE) + SDL_ShowList(&l_tSDLBufMgr, SDL_NODE_STS_BUSY, TRUE) == SDL_BUF_NUM);
    SDL_MemRate(SDL_BUF_NUM, SDL_BUF_SIZE);

    return;
}

/*==============================================================================
Func Name  : L3_BufMgrAllocateNode
Input      : void
Output     : NONE
Return Val : SDL_NODE
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
SDL_NODE *L3_BufMgrAllocateNode(void)
{
    SDL_NODE *ptFreeNode;

    ptFreeNode = SDL_AllocNode(&l_tSDLBufMgr);
    SDL_ASSERT(l_tSDLBufMgr.ptHead->bsFreeCnt + l_tSDLBufMgr.ptHead->bsBsyCnt == SDL_BUF_NUM);
    //SDL_ASSERT(SDL_ShowList(&l_tSDLBufMgr, SDL_NODE_STS_FREE, TRUE) + SDL_ShowList(&l_tSDLBufMgr, SDL_NODE_STS_BUSY, TRUE) == SDL_BUF_NUM);

    return ptFreeNode;
}

/*==============================================================================
Func Name  : L3_BufMgrReleaseNode
Input      : SDL_NODE *ptBsyNode
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void L3_BufMgrReleaseNode(SDL_NODE *ptBsyNode)
{
    SDL_ReleaseNode(&l_tSDLBufMgr, ptBsyNode);
    SDL_ASSERT(l_tSDLBufMgr.ptHead->bsFreeCnt + l_tSDLBufMgr.ptHead->bsBsyCnt == SDL_BUF_NUM);
    //SDL_ASSERT(SDL_ShowList(&l_tSDLBufMgr, SDL_NODE_STS_FREE, TRUE) + SDL_ShowList(&l_tSDLBufMgr, SDL_NODE_STS_BUSY, TRUE) == SDL_BUF_NUM);

    return;
}

/*==============================================================================
Func Name  : L3_BufMgrGetBufIDByNode
Input      : SDL_NODE *ptNode
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
U16 L3_BufMgrGetBufIDByNode(SDL_NODE *ptNode)
{
    U16 usBufID;
    U32 ulBufAddr;

    ulBufAddr = SDL_GetAddrByMemID(l_tSDLBufMgr.ptHead->ulMemBase, ptNode->bsIndex, l_tSDLBufMgr.ptHead->ulUnitSize);
    usBufID = COM_GetBufferIDByMemAddr(ulBufAddr, TRUE, BUF_SIZE_BITS);

    return usBufID;
}

/*==============================================================================
Func Name  : L3_BufMgrGetFreeCnt
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.16 JasonGuo create function
==============================================================================*/
U16 L3_BufMgrGetFreeCnt(void)
{
    return l_tSDLBufMgr.ptHead->bsFreeCnt;
}

/*==============================================================================
Func Name  : L3_RedMgrInit
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_RedMgrInit(void)
{
    SDL_SetListMem(&l_tSDLRedMgr, l_ulSDLRedMgrBase, l_ulSDLRedMemBase);
    SDL_Init(&l_tSDLRedMgr, SDL_RED_NUM, SDL_RED_SIZE);

    SDL_ASSERT(l_tSDLRedMgr.ptHead->bsFreeCnt + l_tSDLRedMgr.ptHead->bsBsyCnt == SDL_RED_NUM);
    //SDL_ASSERT(SDL_ShowList(&l_tSDLRedMgr, SDL_NODE_STS_FREE, TRUE) + SDL_ShowList(&l_tSDLRedMgr, SDL_NODE_STS_BUSY, TRUE) == SDL_RED_NUM);
    SDL_MemRate(SDL_RED_NUM, SDL_RED_SIZE);

    return;
}

/*==============================================================================
Func Name  : L3_RedMgrAllocateNode
Input      : void
Output     : NONE
Return Val : SDL_NODE
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
SDL_NODE *L3_RedMgrAllocateNode(void)
{
    SDL_NODE *ptFreeNode;

    ptFreeNode = SDL_AllocNode(&l_tSDLRedMgr);
    SDL_ASSERT(l_tSDLRedMgr.ptHead->bsFreeCnt + l_tSDLRedMgr.ptHead->bsBsyCnt == SDL_RED_NUM);
    //SDL_ASSERT(SDL_ShowList(&l_tSDLRedMgr, SDL_NODE_STS_FREE, TRUE) + SDL_ShowList(&l_tSDLRedMgr, SDL_NODE_STS_BUSY, TRUE) == SDL_RED_NUM);

    return ptFreeNode;
}

/*==============================================================================
Func Name  : L3_RedMgrReleaseNode
Input      : SDL_NODE *ptBsyNode
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void L3_RedMgrReleaseNode(SDL_NODE *ptBsyNode)
{
    SDL_ReleaseNode(&l_tSDLRedMgr, ptBsyNode);
    SDL_ASSERT(l_tSDLRedMgr.ptHead->bsFreeCnt + l_tSDLRedMgr.ptHead->bsBsyCnt == SDL_RED_NUM);
    //SDL_ASSERT(SDL_ShowList(&l_tSDLRedMgr, SDL_NODE_STS_FREE, TRUE) + SDL_ShowList(&l_tSDLRedMgr, SDL_NODE_STS_BUSY, TRUE) == SDL_RED_NUM);

    return;
}

/*==============================================================================
Func Name  : L3_RedMgrGetRedAddrByNode
Input      : SDL_NODE *ptNode
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
U32 L3_RedMgrGetRedAddrByNode(SDL_NODE *ptNode)
{
    U32 ulRedAddr;

    ulRedAddr = SDL_GetAddrByMemID(l_tSDLRedMgr.ptHead->ulMemBase, ptNode->bsIndex, l_tSDLRedMgr.ptHead->ulUnitSize);

    return ulRedAddr;
}

/*==============================================================================
Func Name  : L3_RedMgrGetFreeCnt
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.16 JasonGuo create function
==============================================================================*/
U16 L3_RedMgrGetFreeCnt(void)
{
    return l_tSDLRedMgr.ptHead->bsFreeCnt;
}

/*====================End of this file========================================*/

