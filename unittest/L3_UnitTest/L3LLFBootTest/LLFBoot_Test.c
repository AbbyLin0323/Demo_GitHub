/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : LLFBoot_Test.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.3.19
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_Xtensa.h"
#include "L1_SCmdInterface.h"
#include "L2_TableBBT.h"
#include "L2_TableBlock.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern BOOL g_BootUpOk;
extern GLOBAL PSCMD g_pCurSCmd;

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_CheckLLFBootDone
Input      : None
Output     : None
Return Val : 
Discription: 
             Handle LLF Cmd and BootUp Cmd             
Usage      : 
History    : 
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
BOOL MCU12_DRAM_TEXT L3_CheckLLFBootDone(void)
{
    BOOL bLLFBOOTDone = FALSE;
    
    if (TRUE == g_BootUpOk)
    {
        DBG_Printf("MCU#%d FAKE SCMD LLF & BOOTUP has done!\n", HAL_GetMcuId());
        return TRUE;
    }

    g_pCurSCmd = L1_GetSCmd();
    if (NULL != g_pCurSCmd)
    {
        if (SCMD_LLF == g_pCurSCmd->ucSCmdType)
        {
            if (TRUE == L2_BbtBuild(FALSE))
            {
                L2_TableBlock_LLF(FALSE);

                DBG_Printf("MCU#%d FAKE SCMD LLF done!\n", HAL_GetMcuId());
                
                L1_SCmdFinish();
            }            
        }
        else if (SCMD_BOOTUP == g_pCurSCmd->ucSCmdType)
        {        
            if (TRUE == L2_BbtLoad(NULL))
            {
                L2_BbtPrintAllBbt();
                DBG_Printf("MCU#%d FAKE SCMD BOOTUP done!\n", HAL_GetMcuId());
                g_BootUpOk = TRUE;
                bLLFBOOTDone = TRUE;
                L1_SCmdFinish();
            }            
        }                
    }

    return bLLFBOOTDone;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_LLFBootTest
Input      : void  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.3.19 JasonGuo create function
==============================================================================*/
BOOL L3_LLFBootTest(void)
{
    BOOL bLLFBootDone;

    bLLFBootDone = L3_CheckLLFBootDone();
    
    if (TRUE == bLLFBootDone)
    {
        DBG_Printf("\nMCU#%d L3_LLFBootTest Finish.\n\n", HAL_GetMcuId());
    }
    
    return bLLFBootDone;
}

/*====================End of this file========================================*/

