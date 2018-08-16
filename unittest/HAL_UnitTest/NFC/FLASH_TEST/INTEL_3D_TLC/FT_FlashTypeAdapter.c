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
extern GLOBAL BOOL g_bFTTlcMode;


/*------------------------------------------------------------------------------
    FUNCTION DEFINITION
------------------------------------------------------------------------------*/
U16 FT_WtGetPageAddr(U16 *pPrgIndex)
{
    U16 usPage = *pPrgIndex;

    if (g_bFTTlcMode)
    {
        if (HIGH_PAGE == HAL_GetFlashPairPageType(usPage))
        {
            //DBG_Printf("program can't from high page %d!\n",usPage);
            usPage++;
            *pPrgIndex = usPage;
        }
    }
    return usPage;
}

#ifdef IM_3D_TLC_B17A
/*------------------------------------------------------------------------------
Name: FT_GetSectionType
Description: Use in B16 flash
Input Param:
Output Param:
Return Value:
    Section type:0-SLC 1-MLC 2-TLC
Usage:
    FT call it to get the section type of special page
History:
    20160626    abby    create
------------------------------------------------------------------------------*/
U8 FT_GetSectionType(U16 usPage)
{
    U8 ucType = INVALID_2F;

    ASSERT(usPage < PG_PER_BLK);
    
    if ((usPage < 12) || (usPage >= 2292))
    {
        ucType = SLC_SEC;
    }
    else if (usPage < 36)
    {
        ucType = MLC_SEC;
    }
    else if (((usPage >= 2222)&&(EXTRA_PAGE != HAL_GetFlashPairPageType(usPage)))&&(usPage < 2268))
    {
        if (LOW_PAGE == HAL_GetFlashPairPageType((usPage+1)%PG_PER_BLK))
            ucType = TLC_SEC;
        else
            ucType = MLC_SEC;
    }
    else
    {
        ucType = TLC_SEC;
    }
    return ucType;
}
#else
/*------------------------------------------------------------------------------
Name: FT_GetSectionType
Description: Use in B0KB flash
Input Param:
Output Param:
Return Value:
    Section type:0-SLC 1-MLC 2-TLC
Usage:
    FT call it to get the section type of special page
History:
    20160626    abby    create
------------------------------------------------------------------------------*/
U8 FT_GetSectionType(U16 usPage)
{
    U8 ucType = INVALID_2F;

    ASSERT(usPage < PG_PER_BLK);
    
    if ((usPage < 16) || (usPage >= 1505 && 1 == usPage%2))
    {
        ucType = SLC_SEC;
    }
    else if (usPage < 48)
    {
        ucType = MLC_SEC;
    }
    else if (((usPage >= 2222)&&(EXTRA_PAGE != HAL_GetFlashPairPageType(usPage)))&&(usPage < 2268))
    {
        if (LOW_PAGE == HAL_GetFlashPairPageType((usPage+1)%PG_PER_BLK))
            ucType = TLC_SEC;
        else
            ucType = MLC_SEC;
    }
    else
    {
        ucType = TLC_SEC;
    }
    return ucType;
}
#endif

U8 FT_GetSharedPageTypeInWL(U16 usPage)
{
    U8 ucPageType = 0;

    if (!g_bFTTlcMode)
    {
        return 0;
    }
    
    if ((usPage < 12)||(usPage >= 2292))//page 0~12,2292~2303
    {
        ucPageType = LP;
    }
    else if (usPage < 36)                    //page 12~35
    {
        ucPageType = (0 == usPage%2) ? LP : UP;
    }
    else if (usPage < 59)                    //page 36~58
    {
        ucPageType = LP;
    }
    else if (usPage < 2222)                  //page 59~2221
    {
        ucPageType = (0 == (usPage - 59) % 3) ? LP : ((1 == (usPage - 59) % 3)? XP : UP);
    }
    else if (usPage < 2268)                  //page 2222~2267
    {
        ucPageType = (0 == (usPage - 2222) % 4) ? LP : ((2 == (usPage - 2222) % 4)? XP : UP);
    }
    else//if (usPage < 2292)                 //page 2268~2291
    {
        ucPageType = (0 == usPage%2) ? XP : UP;
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
Output Param:
    none
Return Value:
    none
Usage:
    setting like read ID
History:
    20170628    abby    create
------------------------------------------------------------------------------*/
BOOL FT_GetFeature(FLASH_ADDR *pFlashAddr, U8 ucAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    U8  ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsDmaByteEn = TRUE;
    pNFCQEntry->aByteAddr.usByteAddr = ucAddr;
    pNFCQEntry->aByteAddr.usByteLength = sizeof(U32);
    pNFCQEntry->bsPrcqStartDw = g_aPrcqTable[NF_PRCQ_GETFEATURE].bsPRCQStartDw;

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_GETFEATURE, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

void FT_SetFeature(U8 ucPU, U8 ucAddr, U32 ulData)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulStatus;
    U32 aFeature[2] = {0, 0};
    
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

    /*  NVDDR2 patch  */
    *(volatile U32*)0x1ff81018 |= (0x1<<30);
    while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun));
    
    /* get feature to confirm flash feature, optional */
    if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
        if (aFeature[0] != ulData)
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value Wrong 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
    }

    return;
}

void FT_FlashConfig(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U8  ucAddr;
    U32 ulData;
    U32 ulStatus;
    U32 aFeature[2] = {0, 0};

    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < 1; tFlashAddr.ucPU++)
    {
        ucAddr = 0x91;
        ulData = 0x104;
        HAL_NfcSetFeature(&tFlashAddr, ulData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
        
        
        /*  NVDDR2 patch  */
        *(volatile U32*)0x1ff81018 |= (0x1<<30);
        
        /* get feature to confirm flash feature, optional */
        ucAddr = 0xDF;
        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
                
        ucAddr = 0x85;
        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
                
        ucAddr = 0x92;
        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }

        ucAddr = 0x91;
        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 0x10;
        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 0x80;

        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 2;
        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 1;
        if (NFC_STATUS_SUCCESS != FT_GetFeature(&tFlashAddr, ucAddr))
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
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
    }
    
    /*  3D patch  */
   // rNfcModeConfig &= (~(0x1<<31));
    //rNfcModeConfig |= (0x1<<31);        //insert ccl enable
    
    //rNfcModeConfig &= (~(0x1));
    //rNfcModeConfig |= (0x1);          //disable cmd insert
    
   // rNfcPgCfg &= (~(0x1<<29));
   // rNfcPgCfg |= (0x1<<29);             //ce hold enable
    
    DBG_Printf("B16 Test Get Feature Done!!\n\n");

    return;
}
#endif

/* end of this file */
