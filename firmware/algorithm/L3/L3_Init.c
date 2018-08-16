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
* File Name    : L3_Init.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_Xtensa.h"
#include "L2_Interface.h"
#include "L3_Interface.h"
#include "L3_Schedule.h"
#include "L3_FCMDQ.h"
#include "L3_ErrHBasic.h"
#include "L3_ErrHExtend.h"
#include "L3_BufMgr.h"
#include "L3_FlashMonitor.h"
#include "L3_Debug.h"
#include "L3_Init.h"

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
/*==============================================================================
Func Name  : L3_AllocSRAM0
Input      : U32 *pFreeSram0Base  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
U32 g_DirtyLPNCntAddr;
void MCU2_DRAM_TEXT L3_TrimAllocSRAM0(U32 *pFreeSram0Base)
{
#ifdef NEW_TRIM
    U32 ulFreeBase = *pFreeSram0Base;
    COM_MemAddr16DWAlign(&ulFreeBase);

    // FLASH_MONITOR_INTR
    g_DirtyLPNCntAddr = ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, VIR_BLK_CNT * 4);
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    DBG_Printf("L3: Trim %dKB, -> %dKB -> totalSRAM0 %dKB\n", (ulFreeBase - *pFreeSram0Base) / 1024, (ulFreeBase - DSRAM0_MCU2_BASE) / 1024, DSRAM0_MCU2_MAX_SIZE / 1024);
    ASSERT(ulFreeBase-DSRAM0_MCU2_BASE < DSRAM0_MCU2_MAX_SIZE);
    *pFreeSram0Base = ulFreeBase;
#endif    
    return;
}

void MCU2_DRAM_TEXT L3_AllocSRAM0(U32 *pFreeSram0Base)
{
    L3_FCMDQAllocSRAM0(pFreeSram0Base);
    
    L3_FMAllocSRAM0(pFreeSram0Base);

    L3_MgrAllcSRAM0(pFreeSram0Base);
    
    L3_TrimAllocSRAM0(pFreeSram0Base);

    return;
}

/*==============================================================================
Func Name  : L3_AllocDRAM
Input      : U32 *pFreeDRAMBase  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_AllocDRAM(U32 *pFreeDRAMBase)
{
    L3_ErrHAllocDram(pFreeDRAMBase);

    L3_ExtHAllcDram(pFreeDRAMBase);

    L3_MgrAllcDram(pFreeDRAMBase);
    
    return;
}


/*==============================================================================
Func Name  : L3_SharedMemMap
Input      : SUBSYSTEM_MEM_BASE *pFreeMemBase  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_SharedMemMap(SUBSYSTEM_MEM_BASE *pFreeMemBase)
{
    L1_SharedMemMap(pFreeMemBase);

    L2_SharedMemMap(pFreeMemBase);

    return;
}

/*==============================================================================
Func Name  : L3_AllocMem
Input      : SUBSYSTEM_MEM_BASE *pFreeMemBase  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_AllocMem(SUBSYSTEM_MEM_BASE *pFreeMemBase)
{
    L3_AllocSRAM0(&pFreeMemBase->ulFreeSRAM0Base);

    L3_AllocDRAM(&pFreeMemBase->ulDRAMBase);

    L3_SharedMemMap(pFreeMemBase);

    return;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_TaskInit
Input      : SUBSYSTEM_MEM_BASE *pFreeMemBase  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_TaskInit(SUBSYSTEM_MEM_BASE *pFreeMemBase)
{
    L3_AllocMem(pFreeMemBase);

    L3_FCMDQIntrInit(FALSE);
    L3_FCMDQIntrInit(TRUE);

    //L3_IFNFCIRSInit();

    L3_ErrHBuildBrthLunMap();

    L3_SchInit();

    L3_FMIntrInit();
    
    L3_BufMgrInit();
    L3_RedMgrInit();

    L3_ErrHAdaptiveRetryTableInit();
    
    L3_DbgInit();
    L2_FCmdReqStsInit(MCU2_ID);

    return;
}

/*====================End of this file========================================*/

