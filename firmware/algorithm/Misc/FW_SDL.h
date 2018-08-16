/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : FW_SDL.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.8.15
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _FW_SDL_H
#define _FW_SDL_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "Disk_Config.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define SDL_INVALID (0xFFFF)
#define SDL_ASSERT(bFlag) if (TRUE != (bFlag)) {DBG_Printf("Line%d.", __LINE__); DBG_Getch();}

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef enum _SDL_NODE_STS_
{
    SDL_NODE_STS_FREE = 0,
    SDL_NODE_STS_BUSY,
    SDL_NODE_STS_NUM
}SDL_NODE_STS;

typedef struct _SDL_HEAD_
{
    U32 bsFreeHeadPtr : 16;
    U32 bsFreeTailPtr : 16;
    U32 bsBsyHeadPtr  : 16;
    U32 bsBsyTailPtr  : 16;
    U32 bsFreeCnt     : 16;
    U32 bsBsyCnt      : 16;
    U32 bsUnitNum     : 16;
    U32 bsRsvd        : 16;
    U32 ulUnitSize;
    U32 ulMemBase;
}SDL_HEAD;

typedef struct _SDL_NODE_
{
    U32 bsStatus : 2;
    U32 bsRsvd   : 14;
    U32 bsIndex  : 16;
    U32 bsPre    : 16;
    U32 bsNext   : 16;
}SDL_NODE, *SDL_LIST;

typedef struct _SDL_MGR_
{
    SDL_HEAD *ptHead;
    SDL_LIST  ptList;
}SDL_MGR;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void SDL_SetListMem(SDL_MGR *ptSDLMgr, U32 ulMgrBase, U32 ulMemBase);
void SDL_Init(SDL_MGR *ptSDLMgr, U32 ulUnitNum, U32 ulUnitSize);
SDL_NODE *SDL_AllocNode(SDL_MGR *ptSDLMgr);
void SDL_ReleaseNode(SDL_MGR *ptSDLMgr, SDL_NODE *ptBsyNode);
U32 SDL_GetAddrByMemID(U32 ulMemBase, U32 ulIndex, U32 ulUnitSize);
U32 SDL_ShowList(SDL_MGR *ptSDLMgr, SDL_NODE_STS eStatus, BOOL bPrint);
void SDL_MemRate(U32 ulNodeNum, U32 ulUnitSize);

#endif
/*====================End of this head file===================================*/

