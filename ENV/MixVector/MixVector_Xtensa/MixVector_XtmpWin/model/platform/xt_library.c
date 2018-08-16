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
********************************************************************************
* File Name    : xt_library.c
* Discription  : 
* CreateAuthor : 
* CreateDate   : 2014.6.26
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include <Windows.h>
#include "BaseDef.h"
#include "xt_library.h"
#include "HAL_Xtensa.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern U32 SIM_GetMCUTreadID(U8 uMCUID);
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
#ifdef SIM_XTENSA
//defined in xtmp_main.c
extern U8 *g_pLocalRam0;
#endif
/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/



/*------------------------------------------------------------------------------
function: void Comm_ReadDram(U32 addr, U32 nWords, U32 *buf);
Description: 
    read data form addr+DRAM_START_ADDRESS to buf
Input Param:
    U32 addr: source offset addr to DRAM BASE
    U32 nWords: copy num 
    U32 *buf: destination addr
Output Param:
    none
Return Value:
    void
Usage:
    when write copy data form DRAM to local buffer in NFC model
History:
------------------------------------------------------------------------------*/
U32 XT_RSR_PRID()
{
    U32 ulThreadID = 0;
    U32 ulMcuID = 0;
    
    ulThreadID = GetCurrentThreadId();

    if (ulThreadID == SIM_GetMCUTreadID(0))
    {
        ulMcuID = MCU0_ID;
    }
    else if (ulThreadID == SIM_GetMCUTreadID(1))
    {
        ulMcuID = MCU1_ID;
    }
    else if(ulThreadID == SIM_GetMCUTreadID(2))
    {
        ulMcuID = MCU2_ID;
    }

    //mix vector is designed as single core mode, and run on MCU1
#ifdef MIX_VECTOR
    ulMcuID = MCU1_ID;
#endif

    return ulMcuID;
}


