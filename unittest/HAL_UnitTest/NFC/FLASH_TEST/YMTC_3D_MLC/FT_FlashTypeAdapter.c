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
Filename    : FT_FlashTypeAdapter.c
Version     : Ver 1.0
Author      : abby
Date        : 20170626
Description : This file defines the special interfaces related flash type.
Others      :
Modify      :
*******************************************************************************/
#ifndef BOOTLOADER
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "FT_Config.h"
#include "FT_FlashTypeAdapter.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_DecStsReport.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    EXTERN DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR PRCQ_TABLE g_aPrcqTable[FLASH_PRCQ_CNT];

/*------------------------------------------------------------------------------
    FUNCTION DEFINITION
------------------------------------------------------------------------------*/
U16 FT_WtGetPageAddr(U16 *pPrgIndex)
{
    return *pPrgIndex;
}

U8 FT_GetSharedPageTypeInWL(U16 usPage)
{
    U8 ucPageType = 0;

    if (!g_bFTMlcMode)
    {
        return 0;
    }
    
    if (0 == usPage/2)//page 0~12,2292~2303
    {
        ucPageType = LP;
    }
    else 
    {
        ucPageType = UP;
    }

    return ucPageType;
}

/*------------------------------------------------------------------------------
Name: FT_GetFeature
Description:
    get flash feature
Input Param:
    U8 ucPU : PU num;
    U8 ucAddr: feature addr
    U32 ulExpectData: expected PA, if none, set it be INVALID_8F
Output Param:
    none
Return Value:
    none
Usage:
    setting like read ID
History:
    20170628    abby    create
------------------------------------------------------------------------------*/
U32 FT_GetFeature(U8 ucPU, U8 ucAddr, U32 ulExpectData)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulStatus;
    U32 aFeature[2] = {0, 0};
    
    tFlashAddr.ucPU = ucPU;

    /*  NVDDR2 patch  */
    *(volatile U32*)0x1ff81018 |= (0x1<<30);
    while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun));
    
    /* get feature to confirm flash feature, optional */
    if (NFC_STATUS_SUCCESS != HAL_NfcGetFeature(&tFlashAddr, ucAddr))
    {
        DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
    }
    else
    { 
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("GetFeature Fail!\n");
        }
        else
        {
            HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
        }
        if (INVALID_8F != ulExpectData) 
        {
            if(aFeature[0] != ulExpectData)
                DBG_Printf("PU %d Get Feature Address 0x%x Feature Value Wrong 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);
            else
                DBG_Printf("PU %d Get Feature Address 0x%x Feature Value Right 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);
        }
        
    }

    return aFeature[0];
}

void FT_SetFeature(U8 ucPU, U8 ucAddr, U32 ulData)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulStatus;
    
    tFlashAddr.ucPU = ucPU;

    /* set feature */
    HAL_NfcSetFeature(&tFlashAddr, ulData, ucAddr);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("set feature fail PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
    }
    else
    {
        //DBG_Printf("set feature OK PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
    }

    FT_GetFeature(ucPU, ucAddr, ulData);

    return;
}

#endif

/* end of this file */
