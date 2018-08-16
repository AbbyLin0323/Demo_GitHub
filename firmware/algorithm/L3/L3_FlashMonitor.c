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
* File Name    : L3_FlashMonitor.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_MemoryMap.h"
#include "L2_FCMDQ.h"
#include "L3_FlashMonitor.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
GLOBAL MCU12_VAR_ATTR FM_USER_ITEM *g_ptFMUserMgr;
LOCAL FM_INTR_ITEM *l_ptFMIntrMgr;

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/

/*==============================================================================
Func Name  : L3_FMAllocSRAM0
Input      : U32 *pFreeSram0Base  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMAllocSRAM0(U32 *pFreeSram0Base)
{
    U32 ulFreeBase = *pFreeSram0Base;
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    // FLASH_MONITOR_INTR
    l_ptFMIntrMgr = (FM_INTR_ITEM *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FM_INTR_TOT_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    DBG_Printf("L3: FMAllcSRAM0 %dKB, -> %dKB -> totalSRAM0 %dKB\n", (ulFreeBase - *pFreeSram0Base) / 1024, (ulFreeBase - DSRAM0_MCU2_BASE) / 1024, DSRAM0_MCU2_MAX_SIZE / 1024);
    ASSERT(ulFreeBase-DSRAM0_MCU2_BASE < DSRAM0_MCU2_MAX_SIZE);
    *pFreeSram0Base = ulFreeBase;
    
    return;
}

/*==============================================================================
Func Name  : L3_FMGetUsrItem
Input      : U8 ucTLun  
Output     : NONE
Return Val : FM_USER_ITEM
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
FM_USER_ITEM *L3_FMGetUsrItem(U8 ucTLun)
{
    return &g_ptFMUserMgr[ucTLun];
}


/*==============================================================================
Func Name  : L3_FMIntrInit
Input      : void  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMIntrInit(void)
{
    U8 ucTLun;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        l_ptFMIntrMgr[ucTLun].bsPhyBlk = INVALID_4F;
        l_ptFMIntrMgr[ucTLun].bsPhyPage = INVALID_4F;
        l_ptFMIntrMgr[ucTLun].bsPln = MSK_F;
        l_ptFMIntrMgr[ucTLun].bsCmdType = 0;

        l_ptFMIntrMgr[ucTLun].bsVthShiftRd = 0;
        l_ptFMIntrMgr[ucTLun].bsSlcVthRetryDft = 0;
        l_ptFMIntrMgr[ucTLun].bsMlcVthRetryDft = 0;

        l_ptFMIntrMgr[ucTLun].bsSlcVthRetry = l_ptFMIntrMgr[ucTLun].bsSlcVthRetryDft;
        l_ptFMIntrMgr[ucTLun].bsMlcVthRetry = l_ptFMIntrMgr[ucTLun].bsMlcVthRetryDft;

        // init to invalid for swith the related mode in the first command. avoid to care about the defaut mode type after power on.
        l_ptFMIntrMgr[ucTLun].bsSLCMode = INVALID_2F; 
    }
    
    return;
}

/*==============================================================================
Func Name  : L3_FMGetPhyBlk
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U16 L3_FMGetPhyBlk(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsPhyBlk;
}

/*==============================================================================
Func Name  : L3_FMGetPhyPage
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U16 L3_FMGetPhyPage(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsPhyPage;
}

/*==============================================================================
Func Name  : L3_FMGetPlnNum
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
1. 2017.6.26 NickLiou create function
==============================================================================*/
U8 L3_FMGetPlnNum(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsPln;
}

/*==============================================================================
Func Name  : L3_FMGetCmdType
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U8 L3_FMGetCmdType(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsCmdType;
}

/*==============================================================================
Func Name  : L3_FMGetVthShiftRd
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U8 MCU2_DRAM_TEXT L3_FMGetVthShiftRd(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsVthShiftRd;
}

/*==============================================================================
Func Name  : L3_FMGetSlcVthRetry
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U8 MCU2_DRAM_TEXT L3_FMGetSlcVthRetry(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsSlcVthRetry;
}

/*==============================================================================
Func Name  : L3_FMGetSlcVthRetryDft
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U8 MCU2_DRAM_TEXT L3_FMGetSlcVthRetryDft(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsSlcVthRetryDft;
}

/*==============================================================================
Func Name  : L3_FMGetMlcVthRetry
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U8 MCU2_DRAM_TEXT L3_FMGetMlcVthRetry(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsMlcVthRetry;
}

/*==============================================================================
Func Name  : L3_FMGetMlcVthRetryDft
Input      : U8 ucTLun  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U8 MCU2_DRAM_TEXT L3_FMGetMlcVthRetryDft(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsMlcVthRetryDft;
}

/*==============================================================================
Func Name  : L3_FMSetPhyBlk
Input      : U8 ucTLun     
             U16 usPhyBlk  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void L3_FMSetPhyBlk(U8 ucTLun, U16 usPhyBlk)
{
    l_ptFMIntrMgr[ucTLun].bsPhyBlk = usPhyBlk;
}

/*==============================================================================
Func Name  : L3_FMSetPhyPage
Input      : U8 ucTLun      
             U16 usPhyPage  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void L3_FMSetPhyPage(U8 ucTLun, U16 usPhyPage)
{
    l_ptFMIntrMgr[ucTLun].bsPhyPage = usPhyPage;
}

/*==============================================================================
Func Name  : L3_FMSetPlnNum
Input      : U8 ucTLun
             U8 ucPln
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
1. 2017.6.26 NickLiou create function
==============================================================================*/
void L3_FMSetPlnNum(U8 ucTLun, U8 ucPln)
{
    l_ptFMIntrMgr[ucTLun].bsPln = ucPln;
}

/*==============================================================================
Func Name  : L3_FMSetCmdType
Input      : U8 ucTLun     
             U8 ucCmdType  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void L3_FMSetCmdType(U8 ucTLun, U8 ucCmdType)
{
    l_ptFMIntrMgr[ucTLun].bsCmdType = ucCmdType;
}

/*==============================================================================
Func Name  : L3_FMSetVthShiftRd
Input      : U8 ucTLun  
             U8 ucVth   
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMSetVthShiftRd(U8 ucTLun, U8 ucVth)
{
    l_ptFMIntrMgr[ucTLun].bsVthShiftRd = ucVth;
}

/*==============================================================================
Func Name  : L3_FMSetSlcVthRetry
Input      : U8 ucTLun  
             U8 ucVth   
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMSetSlcVthRetry(U8 ucTLun, U8 ucVth)
{
    l_ptFMIntrMgr[ucTLun].bsSlcVthRetry = ucVth;
}

/*==============================================================================
Func Name  : L3_FMSetSlcVthRetryDft
Input      : U8 ucTLun  
             U8 ucVth   
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMSetSlcVthRetryDft(U8 ucTLun, U8 ucVth)
{
    l_ptFMIntrMgr[ucTLun].bsSlcVthRetryDft = ucVth;
}

/*==============================================================================
Func Name  : L3_FMSetMlcVthRetry
Input      : U8 ucTLun  
             U8 ucVth   
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMSetMlcVthRetry(U8 ucTLun, U8 ucVth)
{
    l_ptFMIntrMgr[ucTLun].bsMlcVthRetry = ucVth;
}

/*==============================================================================
Func Name  : L3_FMSetMlcVthRetryDft
Input      : U8 ucTLun  
             U8 ucVth   
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMSetMlcVthRetryDft(U8 ucTLun, U8 ucVth)
{
    l_ptFMIntrMgr[ucTLun].bsMlcVthRetryDft = ucVth;
}

/*==============================================================================
Func Name  : L3_FMGetTotErsCnt
Input      : void  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U32 MCU12_DRAM_TEXT L3_FMGetTotErsCnt(void)
{
    U8  ucTLun;
    U32 ulTotErsCnt = 0;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ulTotErsCnt += g_ptFMUserMgr[ucTLun].ulErsTime;
    }

    return ulTotErsCnt;
}

/*==============================================================================
Func Name  : L3_FMGetTotPrgCnt
Input      : void  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
U32 MCU12_DRAM_TEXT L3_FMGetTotPrgCnt(void)
{
    U8  ucTLun;
    U32 ulTotPrgCnt = 0;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ulTotPrgCnt += g_ptFMUserMgr[ucTLun].ulPrgTime;
    }

    return ulTotPrgCnt;
}

/*==============================================================================
Func Name  : L3_FMUpdtUsrOpCnt
Input      : U8 ucTLun      
             U8 ucFCmdType  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void L3_FMUpdtUsrOpCnt(U8 ucTLun, U8 ucFCmdType, U8 ucPairPageCnt)
{
    switch (ucFCmdType)
    {
        case FCMD_REQ_TYPE_READ:
        {
            g_ptFMUserMgr[ucTLun].ulReadTime++;
            break;
        }
        case FCMD_REQ_TYPE_WRITE:
        {
            g_ptFMUserMgr[ucTLun].ulPrgTime += ucPairPageCnt;
            break;
        }
        case FCMD_REQ_TYPE_ERASE:
        {
            g_ptFMUserMgr[ucTLun].ulErsTime++;
            break;
        }
        case FCMD_REQ_TYPE_RDSTS:
        case FCMD_REQ_TYPE_SETFEATURE:
        {
            break;
        }
        default:
        {
            DBG_Printf("TLun%d FCmdType%d Error.\n", ucTLun, ucFCmdType);
            DBG_Getch();
        }
    }
}

/*==============================================================================
Func Name  : L3_FMUpdtUsrFailCnt
Input      : U8 ucTLun     
             U8 ucErrCode  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FMUpdtUsrFailCnt(U8 ucTLun, U8 ucErrCode)
{
    switch (ucErrCode)
    {
        case NF_ERR_TYPE_UECC:
        case NF_ERR_TYPE_DCRC:
        {
            g_ptFMUserMgr[ucTLun].bsUeccErrCnt++;
            break;
        }
        case NF_ERR_TYPE_RECC:
        {
            g_ptFMUserMgr[ucTLun].bsReccErrCnt++;
            break;
        }
        case NF_ERR_TYPE_PRG:
        case NF_ERR_TYPE_PREPRG:
        case NF_ERR_TYPE_BOTHPRG:
        {
            g_ptFMUserMgr[ucTLun].bsPrgErrCnt++;
            break;
        }
        case NF_ERR_TYPE_ERS:
        {
            g_ptFMUserMgr[ucTLun].bsErsErrCnt++;
            break;
        }
        default:
        {
            DBG_Printf("TLun%d ErrCode%d Error.\n", ucTLun, ucErrCode);
            DBG_Getch();
        }
    }

    return;
}

BOOL L3_FMGetSLCMode(U8 ucTLun)
{
    return l_ptFMIntrMgr[ucTLun].bsSLCMode;
}

void L3_FMSetSLCMode(U8 ucTLun, BOOL bSLCMode)
{
    l_ptFMIntrMgr[ucTLun].bsSLCMode = (0 == bSLCMode) ? FALSE : TRUE;
}

/*====================End of this file========================================*/

