/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : FW_SDL.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.8.15
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "FW_SDL.h"

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

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : SDL_SetListMem
Input      : SDL_MGR *ptSDLMgr
             U32 ulMgrBase
             U32 ulMemBase
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void SDL_SetListMem(SDL_MGR *ptSDLMgr, U32 ulMgrBase, U32 ulMemBase)
{
    ptSDLMgr->ptHead = (SDL_HEAD *)ulMgrBase;
    ptSDLMgr->ptList = (SDL_NODE *)(ulMgrBase + sizeof(SDL_HEAD));

    ptSDLMgr->ptHead->ulMemBase = ulMemBase;

    return;
}

/*==============================================================================
Func Name  : SDL_Init
Input      : SDL_MGR *ptSDLMgr
             U32 ulUnitNum
             U32 ulUnitSize
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void SDL_Init(SDL_MGR *ptSDLMgr, U32 ulUnitNum, U32 ulUnitSize)
{
    U32 ulIndex;
    SDL_HEAD *ptHead;
    SDL_LIST  ptList;

    // Head Init
    ptHead = ptSDLMgr->ptHead;
    ptHead->bsFreeHeadPtr = 0;
    ptHead->bsFreeTailPtr = ulUnitNum - 1;
    ptHead->bsBsyHeadPtr = SDL_INVALID;
    ptHead->bsBsyTailPtr = SDL_INVALID;
    ptHead->bsUnitNum = ulUnitNum;
    ptHead->ulUnitSize = ulUnitSize;
    ptHead->bsFreeCnt = ulUnitNum;
    ptHead->bsBsyCnt = 0;

    // List Init
    for (ulIndex = 0; ulIndex < ulUnitNum; ulIndex++)
    {
        ptList = &ptSDLMgr->ptList[ulIndex];
        ptList->bsStatus = SDL_NODE_STS_FREE;
        ptList->bsNext = (ulIndex + 1) % ulUnitNum;
        ptList->bsPre = (ulIndex + ulUnitNum - 1) % ulUnitNum;
        ptList->bsIndex = ulIndex;
    }

    return;
}

/*==============================================================================
Func Name  : SDL_MemRate
Input      : U32 ulNodeNum
             U32 ulUnitSize
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void SDL_MemRate(U32 ulNodeNum, U32 ulUnitSize)
{
    U32 ulMemRate, ulMgrSize, ulMemSize;

    ulMgrSize = sizeof(SDL_MGR) + sizeof(SDL_HEAD) + ulNodeNum * sizeof(SDL_NODE) + 2 * sizeof(U32);
    ulMemSize = ulNodeNum * ulUnitSize;

    ulMemRate = (ulMgrSize + ulMemSize) * 1000 / ulMemSize;

    DBG_Printf("MgrSize=%d, MemSize=%d, Rate=%d.%d%d\n", ulMgrSize, ulMemSize, ulMemRate/1000, ulMemRate%1000/100, ulMemRate%1000%100);

    return;
}

/*==============================================================================
Func Name  : SDL_GetAddrByMemID
Input      : U32 ulMemBase
             U32 ulIndex
             U32 ulUnitSize
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
U32 SDL_GetAddrByMemID(U32 ulMemBase, U32 ulIndex, U32 ulUnitSize)
{
    return ulMemBase + ulIndex * ulUnitSize;
}

/*==============================================================================
Func Name  : SDL_AllocNode
Input      : SDL_MGR *ptSDLMgr
Output     : NONE
Return Val : SDL_NODE
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
SDL_NODE *SDL_AllocNode(SDL_MGR *ptSDLMgr)
{
    U16 usFreePtr;
    SDL_NODE *ptFreeNode;

    usFreePtr = ptSDLMgr->ptHead->bsFreeHeadPtr;
    if (SDL_INVALID == usFreePtr)
    {
        return NULL;
    }

    ptFreeNode = &ptSDLMgr->ptList[usFreePtr];

    SDL_ASSERT(SDL_NODE_STS_FREE == ptFreeNode->bsStatus);
    ptFreeNode->bsStatus = SDL_NODE_STS_BUSY;

    // Del the ptFreeNode from the free link list head
    if (ptSDLMgr->ptHead->bsFreeHeadPtr == ptSDLMgr->ptHead->bsFreeTailPtr)
    {
        ptSDLMgr->ptHead->bsFreeHeadPtr = SDL_INVALID;
        ptSDLMgr->ptHead->bsFreeTailPtr = SDL_INVALID;
    }
    else
    {
        ptSDLMgr->ptHead->bsFreeHeadPtr = ptFreeNode->bsNext;
        ptSDLMgr->ptList[ptFreeNode->bsNext].bsPre = ptFreeNode->bsPre;
        ptSDLMgr->ptList[ptFreeNode->bsPre].bsNext = ptFreeNode->bsNext;
    }

    // Add the ptFreeNode to the busy link list tail
    if (SDL_INVALID == ptSDLMgr->ptHead->bsBsyTailPtr)
    {
        ptSDLMgr->ptHead->bsBsyTailPtr = usFreePtr;
        ptSDLMgr->ptHead->bsBsyHeadPtr = ptSDLMgr->ptHead->bsBsyTailPtr;
        ptFreeNode->bsPre = ptSDLMgr->ptHead->bsBsyTailPtr;
        ptFreeNode->bsNext = ptSDLMgr->ptHead->bsBsyHeadPtr;
    }
    else
    {
        ptFreeNode->bsPre = ptSDLMgr->ptHead->bsBsyTailPtr;
        ptFreeNode->bsNext = ptSDLMgr->ptHead->bsBsyHeadPtr;
        ptSDLMgr->ptList[ptSDLMgr->ptHead->bsBsyHeadPtr].bsPre = usFreePtr;
        ptSDLMgr->ptList[ptSDLMgr->ptHead->bsBsyTailPtr].bsNext = usFreePtr;
        ptSDLMgr->ptHead->bsBsyTailPtr = usFreePtr;
    }

    // update free/busy node cnt
    ptSDLMgr->ptHead->bsFreeCnt--;
    ptSDLMgr->ptHead->bsBsyCnt++;

    return ptFreeNode;
}

/*==============================================================================
Func Name  : SDL_ReleaseNode
Input      : SDL_MGR *ptSDLMgr
             SDL_NODE *ptBsyNode
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
void SDL_ReleaseNode(SDL_MGR *ptSDLMgr, SDL_NODE *ptBsyNode)
{
    U16 usBsyPtr;

    SDL_ASSERT(SDL_INVALID != ptSDLMgr->ptHead->bsBsyHeadPtr);
    SDL_ASSERT(SDL_NODE_STS_BUSY == ptBsyNode->bsStatus);

    usBsyPtr = ptBsyNode->bsIndex;

    // Del the ptBsyNode from the busy liink list
    if (ptSDLMgr->ptHead->bsBsyHeadPtr == ptSDLMgr->ptHead->bsBsyTailPtr)
    {
        SDL_ASSERT(ptSDLMgr->ptHead->bsBsyHeadPtr == usBsyPtr);
        ptSDLMgr->ptHead->bsBsyHeadPtr = SDL_INVALID;
        ptSDLMgr->ptHead->bsBsyTailPtr = SDL_INVALID;
    }
    else
    {
        if (ptSDLMgr->ptHead->bsBsyHeadPtr == usBsyPtr)
        {
            ptSDLMgr->ptHead->bsBsyHeadPtr = ptBsyNode->bsNext;
            ptSDLMgr->ptList[ptBsyNode->bsNext].bsPre = ptBsyNode->bsPre;
            ptSDLMgr->ptList[ptSDLMgr->ptHead->bsBsyTailPtr].bsNext = ptSDLMgr->ptHead->bsBsyHeadPtr;
        }
        else if (ptSDLMgr->ptHead->bsBsyTailPtr == usBsyPtr)
        {
            ptSDLMgr->ptHead->bsBsyTailPtr = ptBsyNode->bsPre;
            ptSDLMgr->ptList[ptBsyNode->bsPre].bsNext = ptBsyNode->bsNext;
            ptSDLMgr->ptList[ptSDLMgr->ptHead->bsBsyHeadPtr].bsPre = ptSDLMgr->ptHead->bsBsyTailPtr;
        }
        else
        {
            ptSDLMgr->ptList[ptBsyNode->bsPre].bsNext = ptBsyNode->bsNext;
            ptSDLMgr->ptList[ptBsyNode->bsNext].bsPre = ptBsyNode->bsPre;
        }
    }

    // Add the ptBsyNode to the free link list tail
    if (SDL_INVALID == ptSDLMgr->ptHead->bsFreeTailPtr)
    {
        ptSDLMgr->ptHead->bsFreeTailPtr = usBsyPtr;
        ptSDLMgr->ptHead->bsFreeHeadPtr = usBsyPtr;
        ptBsyNode->bsPre = ptSDLMgr->ptHead->bsFreeTailPtr;
        ptBsyNode->bsNext = ptSDLMgr->ptHead->bsFreeHeadPtr;
    }
    else
    {
        ptBsyNode->bsPre = ptSDLMgr->ptHead->bsFreeTailPtr;
        ptBsyNode->bsNext = ptSDLMgr->ptHead->bsFreeHeadPtr;
        ptSDLMgr->ptList[ptSDLMgr->ptHead->bsFreeHeadPtr].bsPre = usBsyPtr;
        ptSDLMgr->ptList[ptSDLMgr->ptHead->bsFreeTailPtr].bsNext = usBsyPtr;
        ptSDLMgr->ptHead->bsFreeTailPtr = usBsyPtr;
    }

    ptBsyNode->bsStatus = SDL_NODE_STS_FREE;

    // update free/busy node cnt
    ptSDLMgr->ptHead->bsFreeCnt++;
    ptSDLMgr->ptHead->bsBsyCnt--;

    return;
}

/*==============================================================================
Func Name  : SDL_ShowList
Input      : SDL_MGR *ptSDLMgr
             SDL_NODE_STS eStatus
             BOOL bPrint
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
U32 SDL_ShowList(SDL_MGR *ptSDLMgr, SDL_NODE_STS eStatus, BOOL bPrint)
{
    U16 usCurPtr, usEndPtr;
    U32 ulNodeNum = 0;
    SDL_NODE *ptNode;

    SDL_ASSERT(SDL_NODE_STS_NUM > eStatus);

    if (SDL_NODE_STS_FREE == eStatus)
    {
        usEndPtr = usCurPtr = ptSDLMgr->ptHead->bsFreeHeadPtr;
        if (SDL_INVALID == usCurPtr)
        {
            if (TRUE == bPrint) DBG_Printf("SDL_FreeListEmpty.\n");
            return ulNodeNum;
        }
        else
        {
            if (TRUE == bPrint) DBG_Printf("SDL_FreeList: HeadPtr=%d, TailPtr=%d.\n", ptSDLMgr->ptHead->bsFreeHeadPtr, ptSDLMgr->ptHead->bsFreeTailPtr);
        }
    }
    else
    {
        usEndPtr = usCurPtr = ptSDLMgr->ptHead->bsBsyHeadPtr;
        if (SDL_INVALID == usCurPtr)
        {
            if (TRUE == bPrint) DBG_Printf("SDL_BusyListEmpty.\n");
            return ulNodeNum;
        }
        else
        {
            if (TRUE == bPrint) DBG_Printf("SDL_BusyList: HeadPtr=%d, TailPtr=%d.\n", ptSDLMgr->ptHead->bsBsyHeadPtr, ptSDLMgr->ptHead->bsBsyTailPtr);
        }
    }

    do
    {
        ulNodeNum++;
        ptNode = &ptSDLMgr->ptList[usCurPtr];
        if (TRUE == bPrint) DBG_Printf("%2d <- %2d(0x%x) -> %2d\n", ptNode->bsPre, ptNode->bsIndex, SDL_GetAddrByMemID(ptSDLMgr->ptHead->ulMemBase, ptNode->bsIndex, ptSDLMgr->ptHead->ulUnitSize), ptNode->bsNext);
        usCurPtr = ptSDLMgr->ptList[usCurPtr].bsNext;
    } while (usCurPtr != usEndPtr);

    return ulNodeNum;
}

/*====================End of this file========================================*/

