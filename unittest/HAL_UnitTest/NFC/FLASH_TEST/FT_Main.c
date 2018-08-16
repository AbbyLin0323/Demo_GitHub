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
Filename    : FT_Main.c
Version     : Ver 1.0
Author      : abby
Date        : 20170626
Description : This file is for flash characters test
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "Disk_Config.h"
#include "HAL_Xtensa.h"
#include "FW_BufAddr.h"
#include "HAL_ParamTable.h"
#include "FT_Config.h"
//#include "HAL_FlashDriverBasic.h"
#include "FT_Main.h"
#include "FT_Case.h"
#include "FT_BasicFunc.h"
#include "FT_FlashTypeAdapter.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    EXTERN DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL volatile BOOL g_ErrInjEn;
extern GLOBAL NFC_ERR_INJ g_tErrInj;

/*------------------------------------------------------------------------------
    FUNCTIONS DEFINITION
------------------------------------------------------------------------------*/
/* From Ptable??? pending */
LOCAL void MCU2_DRAM_TEXT FT_GetTestParam(void)
{
#ifdef GET_PARAM_FROM_PTABLE
    //pending
    BOOTLOADER_FILE *pBootLoaderFile;

    pBootLoaderFile = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;

    g_tFTParam = pBootLoaderFile->tSysParameterTable.tFTParam;
    
    g_bFTGetParamExternal = TRUE;
    
#else
    g_bFTGetParamExternal = FALSE;

    g_bFTMlcMode        = TRUE;
    g_bFTSnapRead       = FALSE;      //default is not snap read
    g_bFTRawDataRead    = FALSE;      //default is raw data read
    g_ErrInjEn          = FALSE;      //default is no error injection
    g_bFTDecFifoEn      = FALSE;      //default disable DEC FIFO status report
    g_ucFTDecFifoRp     = 0;
    g_bFTSinglePgProg   = FALSE;      //default disable MLC single page program
#endif
}

/****************************************************************************
Function      : FT_Main
Input         : 
Output        :
Description   : Init test mode/parameter/mem map/data prepare
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
LOCAL void MCU2_DRAM_TEXT FT_Init(void)
{
    FT_GetTestParam();
    
    FT_DataDRAMMemMap();
}   

/****************************************************************************
Function      : FT_Main
Input         : 
Output        :
Description   : The entrance of flash test
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
void FT_Main(void)
{
    U32 ulLoop = 0;
    
    /* Initialization at first */
    FT_Init();
    DBG_Printf("FT_Init Done!\n");

#ifdef GET_DEFAULT_FEAT
    FT_GetDefaultFeat();
    DBG_Printf("FT Get Default Flash Feature Done!\n");
#endif

#ifdef IDB_READ
    /* read IDB test */
    FT_ReadIDB();
    DBG_Printf("Read IDB Done!\n\n"); 

    g_bFTMlcMode = FALSE; 
    FT_EraseAllBlk(); //only erase
    DBG_Printf("1 PLN SLC FT_EraseAllBlk PASS!\n\n");
    
    FT_ReadIDB();
    DBG_Printf("Read IDB After Erase Done!\n\n");  

    while(1);
#endif

#ifdef OTP_TEST
    FT_OTPRead();
    DBG_Printf("OTP Read Done!\n\n");
    
    while(1);
#endif

#ifdef SNAP_READ_TEST
    g_bFTMlcMode = FALSE;
    FT_SnapRead();
    DBG_Printf("SLC 1 PLN FT_SnapRead PASS!\n\n");

    g_bFTMlcMode = TRUE;
    FT_SnapRead(); 
    DBG_Printf("MLC 1 PLN FT_SnapRead PASS!\n\n");

    while(1);
#endif

#ifdef SLC_SHIFT_ADDR_TEST
    FT_SLCShiftAddrTest();
    DBG_Printf("SLC Shift Page Address Test PASS!\n\n");

    while(1);
#endif

#ifdef MLC_SINGLE_PAGE_MODE
    FT_MLCSinglePageProg();
    DBG_Printf("MLC Single Page Program Test PASS!\n\n");

    while(1);
#endif

    /* start to burn-in test, select case group as you need */
    while(1)
    {
        DBG_Printf("\nFLASH TEST LOOP %d START!\n" , ulLoop);

        g_bFTRawDataRead = FALSE;
#if 1
        /* single-plane test */        
        g_bFTMlcMode = TRUE;
        FT_BasicEWR();
        DBG_Printf("MLC 1 PLN FT_BasicEWR PASS!\n\n");
        
        //FT_MultiPuEWR();
        //DBG_Printf("MLC 1 PLN FT_MultiPuEWR PASS!\n\n"); 
#endif

#if 1
        g_bFTMlcMode = FALSE;       
        FT_BasicEWR();
        DBG_Printf("SLC 1 PLN FT_BasicEWR PASS!\n\n");
        
        //FT_MultiPuEWR();
        //DBG_Printf("SLC 1 PLN FT_MultiPuEWR PASS!\n\n"); 
#endif
        DBG_Printf("\nFLASH TEST LOOP %d END!\n\n", ulLoop++);
        while (1);
    }
}

/* end of this file */
