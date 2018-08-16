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
* File Name    : L3_BufMgr.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_BUFMGR_H
#define _L3_BUFMGR_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "FW_SDL.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
/********************************************/
/* APP1: Dram Buffer for BlkSwap Management */
/********************************************/
#define SDL_BUF_NUM  (16 + SUBSYSTEM_LUN_NUM)
#define SDL_BUF_SIZE (BUF_SIZE)

/********************************/
/* APP2: Red Resource Management */
/********************************/
#define SDL_RED_NUM  (PG_PER_SLC_BLK * 2)
#define SDL_RED_SIZE (RED_SZ * PLN_PER_LUN * PG_PER_WL)

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
// Memory Allocation
void L3_MgrAllcDram(U32 *pFreeDramBase);
void L3_MgrAllcSRAM0(U32 *pFreeSram0Base);

void MCU2_DRAM_TEXT L3_BufMgrInit(void);
SDL_NODE *L3_BufMgrAllocateNode(void);
void L3_BufMgrReleaseNode(SDL_NODE *ptBsyNode);
U16 L3_BufMgrGetBufIDByNode(SDL_NODE *ptNode);
U16 L3_BufMgrGetFreeCnt(void);

void MCU2_DRAM_TEXT L3_RedMgrInit(void);
SDL_NODE *L3_RedMgrAllocateNode(void);
void L3_RedMgrReleaseNode(SDL_NODE *ptBsyNode);
U32 L3_RedMgrGetRedAddrByNode(SDL_NODE *ptNode);
U16 L3_RedMgrGetFreeCnt(void);

#endif
/*====================End of this head file===================================*/

