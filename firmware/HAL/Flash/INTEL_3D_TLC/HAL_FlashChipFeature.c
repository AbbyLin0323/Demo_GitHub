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
Filename    : HAL_FlashChipFeature.c
Version     :
Author      : Tobey
Date        :
Description : This file include flash feature init, retry and pair page map
              function, which is closely related to certain flash type
Others      :
Modify      :
20160416    abby    create
*******************************************************************************/
#include "HAL_FlashDriverBasic.h"
#include "BaseDef.h"

#if defined(BOOTLOADER)
#include "BootLoader.h" //Henry add for BL part0 retry
#endif

#include "HAL_LdpcEngine.h"
#include "HAL_HostInterface.h"
#ifdef SIM
#include "sim_flash_shedule.h"
#endif

LOCAL U16  l_aSLCPageMap[PG_PER_BLK/3];
LOCAL U8 ScanVtOffSet[VT_SCAN_CNT] = {0xE6, 0xE8, 0xEA, 0xEC, 0xEE, 0xF0, 0xF2, 0xF4, 0xF6, 0xF8, 0xFA, 0xFC, 0xFE,  0, 2, 4, 6, 8, 10};


//static retry para table, actural value pending for more test data
#ifdef READ_RETRY_REFACTOR
/* Read Retry sent para */
/* ==== For Block_Mode : SLC, Page_Type : Low without High Page ==== */
LOCAL RETRY_TABLE g_aSlcRetryPara[SLC_RETRY_GROUP_CNT] =
{
    { 0xEF, 0xA4, 0xF6, INVALID_2F },
    { 0xEF, 0xA4, 0xEC, INVALID_2F },
    { 0xEF, 0xA4, 0xFB, INVALID_2F },
    { 0xEF, 0xA4, 0x05, INVALID_2F },
    { 0xEF, 0xA4, 0xF1, INVALID_2F },
    { 0xEF, 0xA4, 0x0A, INVALID_2F },
    { 0xEF, 0xA4, 0xE7, INVALID_2F },
    { 0xEF, 0xA4, 0xE2, INVALID_2F },
    { 0xEF, 0xA4, 0x0F, INVALID_2F },
    { 0xEF, 0xA4, 0x14, INVALID_2F },
    { 0xEF, 0xA4, 0x19, INVALID_2F },
    { 0xEF, 0xA4, 0xDD, INVALID_2F },
    { 0xEF, 0xA4, 0x1E, INVALID_2F },
    { 0xEF, 0xA4, 0x23, INVALID_2F },
    { 0xEF, 0xA4, 0xD8, INVALID_2F },
    { 0xEF, 0xA4, 0x28, INVALID_2F },
};

/* ==== For Block_Mode : TLC, WL_Type : MLC, Page_Type: Low Page ==== */
LOCAL RETRY_TABLE g_aMlcRetryParaForLP[MLC_RETRY_GROUP_CNT] =
{
    { 0xEF, 0xA1, 0xF6, INVALID_2F },
    { 0xEF, 0xA1, 0xEC, INVALID_2F },
    { 0xEF, 0xA1, 0xFB, INVALID_2F },
    { 0xEF, 0xA1, 0x05, INVALID_2F },
    { 0xEF, 0xA1, 0xF1, INVALID_2F },
    { 0xEF, 0xA1, 0x0A, INVALID_2F },
    { 0xEF, 0xA1, 0xE7, INVALID_2F },
    { 0xEF, 0xA1, 0xE2, INVALID_2F },
    { 0xEF, 0xA1, 0x0F, INVALID_2F },
    { 0xEF, 0xA1, 0x14, INVALID_2F },
    { 0xEF, 0xA1, 0x19, INVALID_2F },
    { 0xEF, 0xA1, 0xDD, INVALID_2F },
    { 0xEF, 0xA1, 0x1E, INVALID_2F },
    { 0xEF, 0xA1, 0x23, INVALID_2F },
    { 0xEF, 0xA1, 0xD8, INVALID_2F },
    { 0xEF, 0xA1, 0x28, INVALID_2F },
};

/* ==== For Block_Mode : TLC, WL_Type : MLC, Page_Type: Upper Page ==== */
LOCAL RETRY_TABLE g_aMlcRetryParaForUP[MLC_RETRY_GROUP_CNT][2] =
{
    { { 0xEF, 0xA0, 0xF1, INVALID_2F }, { 0xEF, 0xA2, 0xF1, INVALID_2F } },
    { { 0xEF, 0xA0, 0xF1, INVALID_2F }, { 0xEF, 0xA2, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA0, 0xFB, INVALID_2F }, { 0xEF, 0xA2, 0xF1, INVALID_2F } },
    { { 0xEF, 0xA0, 0xEC, INVALID_2F }, { 0xEF, 0xA2, 0xEC, INVALID_2F } },
    { { 0xEF, 0xA0, 0xF6, INVALID_2F }, { 0xEF, 0xA2, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA0, 0xFB, INVALID_2F }, { 0xEF, 0xA2, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA0, 0xF6, INVALID_2F }, { 0xEF, 0xA2, 0x05, INVALID_2F } },
    { { 0xEF, 0xA0, 0xFB, INVALID_2F }, { 0xEF, 0xA2, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA0, 0xFB, INVALID_2F }, { 0xEF, 0xA2, 0x05, INVALID_2F } },
    { { 0xEF, 0xA0, 0x00, INVALID_2F }, { 0xEF, 0xA2, 0x0A, INVALID_2F } },
    { { 0xEF, 0xA0, 0x05, INVALID_2F }, { 0xEF, 0xA2, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA0, 0x05, INVALID_2F }, { 0xEF, 0xA2, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA0, 0x0A, INVALID_2F }, { 0xEF, 0xA2, 0x0A, INVALID_2F } },
    { { 0xEF, 0xA0, 0x05, INVALID_2F }, { 0xEF, 0xA2, 0x05, INVALID_2F } },
    { { 0xEF, 0xA0, 0x0A, INVALID_2F }, { 0xEF, 0xA2, 0x00, INVALID_2F } },
    { { 0xEF, 0xA0, 0x0A, INVALID_2F }, { 0xEF, 0xA2, 0xFB, INVALID_2F } },
};

/* ==== For Block_Mode : TLC, WL_Type : TLC, Page_Type: Low Page ==== */
LOCAL RETRY_TABLE g_aTlcRetryParaForLP[TLC_RETRY_GROUP_CNT] =
{
    { 0xEF, 0xA8, 0xF6, INVALID_2F },
    { 0xEF, 0xA8, 0xEC, INVALID_2F },
    { 0xEF, 0xA8, 0xFB, INVALID_2F },
    { 0xEF, 0xA8, 0x05, INVALID_2F },
    { 0xEF, 0xA8, 0xF1, INVALID_2F },
    { 0xEF, 0xA8, 0x0A, INVALID_2F },
    { 0xEF, 0xA8, 0xE7, INVALID_2F },
    { 0xEF, 0xA8, 0xE2, INVALID_2F },
    { 0xEF, 0xA8, 0x0F, INVALID_2F },
    { 0xEF, 0xA8, 0x14, INVALID_2F },
    { 0xEF, 0xA8, 0x19, INVALID_2F },
    { 0xEF, 0xA8, 0xDD, INVALID_2F },
    { 0xEF, 0xA8, 0x1E, INVALID_2F },
    { 0xEF, 0xA8, 0x23, INVALID_2F },
    { 0xEF, 0xA8, 0xD8, INVALID_2F },
    { 0xEF, 0xA8, 0x28, INVALID_2F },
};

/* ==== For Block_Mode : TLC, WL_Type : TLC, Page_Type: Upper Page ==== */
LOCAL RETRY_TABLE g_aTlcRetryParaForUP[TLC_RETRY_GROUP_CNT][2] =
{
    { { 0xEF, 0xA6, 0xF1, INVALID_2F }, { 0xEF, 0xAA, 0xF1, INVALID_2F } },
    { { 0xEF, 0xA6, 0xF1, INVALID_2F }, { 0xEF, 0xAA, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA6, 0xFB, INVALID_2F }, { 0xEF, 0xAA, 0xF1, INVALID_2F } },
    { { 0xEF, 0xA6, 0xEC, INVALID_2F }, { 0xEF, 0xAA, 0xEC, INVALID_2F } },
    { { 0xEF, 0xA6, 0xF6, INVALID_2F }, { 0xEF, 0xAA, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA6, 0xFB, INVALID_2F }, { 0xEF, 0xAA, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA6, 0xF6, INVALID_2F }, { 0xEF, 0xAA, 0x05, INVALID_2F } },
    { { 0xEF, 0xA6, 0xFB, INVALID_2F }, { 0xEF, 0xAA, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA6, 0xFB, INVALID_2F }, { 0xEF, 0xAA, 0x05, INVALID_2F } },
    { { 0xEF, 0xA6, 0x00, INVALID_2F }, { 0xEF, 0xAA, 0x0A, INVALID_2F } },
    { { 0xEF, 0xA6, 0x05, INVALID_2F }, { 0xEF, 0xAA, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA6, 0x05, INVALID_2F }, { 0xEF, 0xAA, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA6, 0x0A, INVALID_2F }, { 0xEF, 0xAA, 0x0A, INVALID_2F } },
    { { 0xEF, 0xA6, 0x05, INVALID_2F }, { 0xEF, 0xAA, 0x05, INVALID_2F } },
    { { 0xEF, 0xA6, 0x0A, INVALID_2F }, { 0xEF, 0xAA, 0x00, INVALID_2F } },
    { { 0xEF, 0xA6, 0x0A, INVALID_2F }, { 0xEF, 0xAA, 0xFB, INVALID_2F } },
};

/* ==== For Block_Mode : TLC, WL_Type : TLC, Page_Type: Extra Page ==== */
LOCAL RETRY_TABLE g_aTlcRetryParaForXP[TLC_RETRY_GROUP_CNT][4] =
{
    { { 0xEF, 0xA5, 0x00, INVALID_2F }, { 0xEF, 0xA7, 0x00, INVALID_2F }, { 0xEF, 0xA9, 0xFB, INVALID_2F }, { 0xEF, 0xAB, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA5, 0x00, INVALID_2F }, { 0xEF, 0xA7, 0xFB, INVALID_2F }, { 0xEF, 0xA9, 0xFB, INVALID_2F }, { 0xEF, 0xAB, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA5, 0xFB, INVALID_2F }, { 0xEF, 0xA7, 0xF6, INVALID_2F }, { 0xEF, 0xA9, 0xF6, INVALID_2F }, { 0xEF, 0xAB, 0xF1, INVALID_2F } },
    { { 0xEF, 0xA5, 0x05, INVALID_2F }, { 0xEF, 0xA7, 0x05, INVALID_2F }, { 0xEF, 0xA9, 0x05, INVALID_2F }, { 0xEF, 0xAB, 0x05, INVALID_2F } },
    { { 0xEF, 0xA5, 0x05, INVALID_2F }, { 0xEF, 0xA7, 0x05, INVALID_2F }, { 0xEF, 0xA9, 0x0A, INVALID_2F }, { 0xEF, 0xAB, 0x0A, INVALID_2F } },
    { { 0xEF, 0xA5, 0x05, INVALID_2F }, { 0xEF, 0xA7, 0x00, INVALID_2F }, { 0xEF, 0xA9, 0xFB, INVALID_2F }, { 0xEF, 0xAB, 0xF6, INVALID_2F } },
    { { 0xEF, 0xA5, 0xF6, INVALID_2F }, { 0xEF, 0xA7, 0xF6, INVALID_2F }, { 0xEF, 0xA9, 0xF1, INVALID_2F }, { 0xEF, 0xAB, 0xF1, INVALID_2F } },
    { { 0xEF, 0xA5, 0x0A, INVALID_2F }, { 0xEF, 0xA7, 0x00, INVALID_2F }, { 0xEF, 0xA9, 0xFB, INVALID_2F }, { 0xEF, 0xAB, 0xFB, INVALID_2F } },
    { { 0xEF, 0xA5, 0xFB, INVALID_2F }, { 0xEF, 0xA7, 0xFB, INVALID_2F }, { 0xEF, 0xA9, 0x05, INVALID_2F }, { 0xEF, 0xAB, 0x05, INVALID_2F } },
    { { 0xEF, 0xA5, 0xF1, INVALID_2F }, { 0xEF, 0xA7, 0xF1, INVALID_2F }, { 0xEF, 0xA9, 0xF1, INVALID_2F }, { 0xEF, 0xAB, 0xF1, INVALID_2F } },
    { { 0xEF, 0xA5, 0x00, INVALID_2F }, { 0xEF, 0xA7, 0x00, INVALID_2F }, { 0xEF, 0xA9, 0x05, INVALID_2F }, { 0xEF, 0xAB, 0x05, INVALID_2F } },
    { { 0xEF, 0xA5, 0x0A, INVALID_2F }, { 0xEF, 0xA7, 0x05, INVALID_2F }, { 0xEF, 0xA9, 0x05, INVALID_2F }, { 0xEF, 0xAB, 0x05, INVALID_2F } },
    { { 0xEF, 0xA5, 0x0A, INVALID_2F }, { 0xEF, 0xA7, 0x0A, INVALID_2F }, { 0xEF, 0xA9, 0x0A, INVALID_2F }, { 0xEF, 0xAB, 0x0A, INVALID_2F } },
    { { 0xEF, 0xA5, 0xFB, INVALID_2F }, { 0xEF, 0xA7, 0x00, INVALID_2F }, { 0xEF, 0xA9, 0x05, INVALID_2F }, { 0xEF, 0xAB, 0x05, INVALID_2F } },
    { { 0xEF, 0xA5, 0xF6, INVALID_2F }, { 0xEF, 0xA7, 0xFB, INVALID_2F }, { 0xEF, 0xA9, 0x00, INVALID_2F }, { 0xEF, 0xAB, 0x05, INVALID_2F } },
    { { 0xEF, 0xA5, 0xFB, INVALID_2F }, { 0xEF, 0xA7, 0xFB, INVALID_2F }, { 0xEF, 0xA9, 0x00, INVALID_2F }, { 0xEF, 0xAB, 0x0A, INVALID_2F } },
};
#endif

LOCAL MCU12_DRAM_TEXT RETRY_TABLE g_aRetryPara[RETRY_CNT] =
{
    /* MICRON L95 MLC:SET FEATURE */
    {   0xEF, 0x89, 0x0, INVALID_2F     },
    {   0xEF, 0x89, 0x1, INVALID_2F     },
    {   0xEF, 0x89, 0x2, INVALID_2F     },
    {   0xEF, 0x89, 0x3, INVALID_2F     },
    {   0xEF, 0x89, 0x4, INVALID_2F     },
    {   0xEF, 0x89, 0x5, INVALID_2F     },
    {   0xEF, 0x89, 0x6, INVALID_2F     },
    {   0xEF, 0x89, 0x7, INVALID_2F     },
    {   0xEF, 0x89, 0x8, INVALID_2F     },
    {   0xEF, 0x89, 0x9, INVALID_2F     },
    {   0xEF, 0x89, 0xA, INVALID_2F     },
    {   0xEF, 0x89, 0xB, INVALID_2F     },
    {   0xEF, 0x89, 0xC, INVALID_2F     },
    {   0xEF, 0x89, 0xD, INVALID_2F     },
    {   0xEF, 0x89, 0xE, INVALID_2F     },
    {   0xEF, 0x89, 0xF, INVALID_2F     }
};

#ifndef SIM
//Henry add. For BL Part0 retry. (Must be located in OTFB)
RETRY_TABLE g_aRetryParaForPart0[RETRY_CNT] =
{
    /* MICRON L95 MLC:SET FEATURE */
    {   0xEF, 0x89, 0x0, INVALID_2F     },
    {   0xEF, 0x89, 0x1, INVALID_2F     },
    {   0xEF, 0x89, 0x2, INVALID_2F     },
    {   0xEF, 0x89, 0x3, INVALID_2F     },
    {   0xEF, 0x89, 0x4, INVALID_2F     },
    {   0xEF, 0x89, 0x5, INVALID_2F     },
    {   0xEF, 0x89, 0x6, INVALID_2F     },
    {   0xEF, 0x89, 0x7, INVALID_2F     },
    {   0xEF, 0x89, 0x8, INVALID_2F     },
    {   0xEF, 0x89, 0x9, INVALID_2F     },
    {   0xEF, 0x89, 0xA, INVALID_2F     },
    {   0xEF, 0x89, 0xB, INVALID_2F     },
    {   0xEF, 0x89, 0xC, INVALID_2F     },
    {   0xEF, 0x89, 0xD, INVALID_2F     },
    {   0xEF, 0x89, 0xE, INVALID_2F     },
    {   0xEF, 0x89, 0xF, INVALID_2F     }
};
#endif

/*------------------------------------------------------------------------------
    LOCAL FUNCTION DECLARATION
------------------------------------------------------------------------------*/

U8 MCU12_DRAM_TEXT HAL_FlashNfcFeatureGetScanVt(U8 ucIdx)
{
    return ScanVtOffSet[ucIdx];
}

/*------------------------------------------------------------------------------
Name: HAL_FlashNfcFeatureInit
Description:
    Initialize additional features which are needed by Normal FW;
    Some feature should init in scripts.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in normal FW initialization sequence
History:
    20160415    abby   create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashNfcFeatureInit(void)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;

    /*  MULTI-LUN related  */
    pNfcPgCfg->bsMulLun = NFC_LUN_PER_PU_BITS;
    if (0 != NFC_LUN_PER_PU_BITS)
    {
        /* DELAY_ACC_EN Disable when MUL_LUN(1ff8_1000[28:27]) not set to 0 */
        pNfcPgCfg->bsDelayAccEn = FALSE;
        /* MICRON flash   */
        pNfcPgCfg->bsMulLunToggle = FALSE;
        pNfcPgCfg->bsLunBdEn = FALSE;
    }
    else
    {
        pNfcPgCfg->bsDelayAccEn = TRUE;
    }

    /*  CRC enable  */
    pNfcPgCfg->bsDCrcChkEn = FALSE;
    pNfcPgCfg->bsNcrcChkEn = TRUE;

    /*  CE decode need enable it  */
    if (0 != NFC_CE_PER_PU_BITS)
    {
        pNfcPgCfg->bsAccProgMode = TRUE;
    }

    /*  CDC issue patch in ICB  */
    pNfcPgCfg->bsIcbRdy = FALSE;//TRUE;

    /*  scramble enable    */
    pNfcPgCfg->bsScrBps = SCRAMBLE_MSK_EN;

    /*  Enable OTFBM ADS bus and write back when program done, need set in PCIE mode    */
#ifdef HOST_SATA
    pNfcPgCfg->bsFstDatRdyMod = TRUE;
    pNfcPgCfg->bsOtfbmEn = FALSE;
    pNfcPgCfg->bsWbPcieMode = FALSE;
#elif defined(HOST_NVME)
    pNfcPgCfg->bsOtfbmEn = TRUE;
    pNfcPgCfg->bsWbPcieMode = TRUE;
#endif

    pNfcPgCfg->bsRTSB3dTlc = TRUE;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashInitSLCMapping
Description:
    Initialize SLC logic page to MLC physical page map.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in normal FW initialization sequence
History:
    20160415    abby   create
    20160526    steven  Intel 3D TLC
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashInitSLCMapping(void)
{
    U32 ulLogicPage;
    
    for(ulLogicPage = 0; ulLogicPage < (PG_PER_BLK/3); ulLogicPage++)
    {
        l_aSLCPageMap[ulLogicPage] = ulLogicPage;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetFlashPairPageType
Description:
    Get intel TLC page type in a pair of page.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in L2 FW
History:
    20150121    abby    add header
    20160526    steven  Intel 3D TLC
    20170330    yanfei  change if..if to if..else if
------------------------------------------------------------------------------*/
PAIR_PAGE_TYPE HAL_GetFlashPairPageType(U16 usPage)
{
    PAIR_PAGE_TYPE ePairPageType;

#ifdef FLASH_IM_3DTLC_GEN2

    if ((usPage < 12)||(usPage >= 2292))//page 0~12,2292~2303
    {
        return LOW_PAGE_WITHOUT_HIGH;
    }
    else if (usPage < 36)                    //page 12~35
    {
        if (0 == usPage%2)
            return LOW_PAGE;
        else
            return HIGH_PAGE;
    }
    else if (usPage < 59)                    //page 36~58
    {
        return LOW_PAGE;
        //return LOW_PAGE_WITHOUT_HIGH;
    }
    else if (usPage < 2222)                  //page 59~2221
    {
        if (0 == (usPage - 59) % 3)     
            return LOW_PAGE;
        else if (1 == (usPage - 59) % 3)
            return EXTRA_PAGE;
        else
            return HIGH_PAGE;
    }
    else if (usPage < 2268)                  //page 2222~2267
    {
        if (0 == (usPage - 2222) % 4)     
            return LOW_PAGE;
        else if (2 == (usPage - 2222) % 4)
            return EXTRA_PAGE;
        else //1 or 3 = (usPage - 2222) % 4
            return HIGH_PAGE;
    }
    else// if (usPage < 2292)                  //page 2268~2291
    {
        if (0 == usPage % 2)     
            return EXTRA_PAGE;
        else
            return HIGH_PAGE;
    }

#else //B0KB

    if(usPage < 16)
    {
        ePairPageType = LOW_PAGE_WITHOUT_HIGH;
    }
    
    if((usPage >= 16) && (usPage < 112))
    {
        if (EVEN == usPage % 2)
        {
            ePairPageType = LOW_PAGE;
        }
        else
        {
            ePairPageType = HIGH_PAGE;
        }
    }
    
    if((usPage >= 112) && (usPage < 1505))
    {
        if (0 == (usPage - 112) % 3)
        {
            ePairPageType = EXTRA_PAGE;
        }
        else if (1 == (usPage - 112) % 3)
        {
            ePairPageType = LOW_PAGE;
        }
        else
        {
            ePairPageType = HIGH_PAGE;
        }
    }
    if((usPage >= 1505) && (usPage < 1536))
    {
        if (0 == usPage % 2)
        {
            ePairPageType = EXTRA_PAGE;
        }
        else
        {
            ePairPageType = LOW_PAGE_WITHOUT_HIGH;
        }
    }
    
    return ePairPageType;
#endif
}

WL_TYPE HAL_GetFlashWlType(U16 usPage, BOOL bsTLC)
{
    U8 ucWLType;

    if (TRUE == bsTLC)
    {
        if (usPage < 16)
        {
            ucWLType = MLC_TYPE;
        }

        else if ((usPage >= 16) && (usPage < 48))
        {
            ucWLType = MLC_TYPE;
        }
        else if ((usPage >= 1457) && (usPage < 1504) && (1 != usPage % 3))
        {
            ucWLType = MLC_TYPE;
        }
        else if ((usPage >= 1505) && (usPage < 1536) && (0 != usPage % 2))
        {
            ucWLType = SLC_TYPE;
        }
        else
        {
            ucWLType = TLC_TYPE;
        }
    }
    else
    {
        ucWLType = SLC_TYPE;
    }
#endif

    return ucWLType;
}

U16 HAL_GetHighPageIndexfromExtra(U16 usExtraPg)
{
    U16 usHighPg = INVALID_4F;

    if(usExtraPg == 112)
    {
        usHighPg = 49;
    }
    else if (((usExtraPg - 112) / 3) < 32)
    {
        usHighPg = 49 + (((usExtraPg - 112) / 3) * 2);
    }
    else if((((usExtraPg - 112) / 3) >= 32) && (usExtraPg < 1456))
    {
        usHighPg = 49 + (31 * 2) + ((((usExtraPg - 112) / 3) - 31) * 3);
    }
    else if((usExtraPg >= 1456) && (usExtraPg <= 1504))
    {
        usHighPg = 1362 + (usExtraPg - 1456);
    }
    else if(usExtraPg > 1504)
    {
        usHighPg = (((usExtraPg - 1504) / 2) * 3) + 1410;
    }

    if(INVALID_4F == usHighPg)
    {
        DBG_Printf("Get upper page error!!!\n");
        DBG_Getch();
    }

    return usHighPg;
}

#ifdef FLASH_IM_3DTLC_GEN2
U16 HAL_GetLowPageIndexfromHigh(U16 usHighPg)
{
    U16 usLowhPg = INVALID_4F;

    if (usHighPg < 36)
    {
        usLowhPg = usHighPg - 1;
    }
    if (usHighPg >= 61 && usHighPg <= 130)
    {
        usLowhPg = usHighPg - ((usHighPg - 61)*3)/2 - 25;
    }
    else if (usHighPg >= 133 && usHighPg <= 2221)
    {
        usLowhPg = usHighPg - 71;
    }
    else if (usHighPg >= 2225 && usHighPg <= 2269 && (usHighPg%4) == 1)
    {
        usLowhPg = usHighPg - 72 - (usHighPg - 2225)/4;
    }
    else if (usHighPg >= 2223 && usHighPg <= 2267 && (usHighPg%4) == 3)
    {
        usLowhPg = usHighPg - 1;
    }
    else if(usHighPg >= 2271 && usHighPg <= 2291)
    {
        usLowhPg = usHighPg - 82 + (usHighPg - 2271)/2;
    }

    if (INVALID_4F == usHighPg)
    {
        DBG_Printf("Get lower page error!!!\n");
        DBG_Getch();
    }

    return usLowhPg;
}
#endif

/*------------------------------------------------------------------------------
Name: HAL_GetLowPageIndex
Description:
    Get low page number from high page in a pair of
Input Param:
    U16 usHighPage: high page number
Output Param:
    none
Return Value:
    U16 usLowPage
Usage:
History:
    20160116    abby     add the header, meet to TLC->just return
    20160526    steven  Intel 3D TLC
------------------------------------------------------------------------------*/
U16 HAL_GetLowPageIndex(U16 usPage)
{
    U16 usLowPage = INVALID_4F;
    U16 usHighPage = INVALID_4F;
    PAIR_PAGE_TYPE ucPageType;

    ucPageType = HAL_GetFlashPairPageType(usPage);

    switch(ucPageType)
    {
        case LOW_PAGE:
        {
            usLowPage = usPage;
            break;
        }
        case HIGH_PAGE:
        {
#ifdef FLASH_IM_3DTLC_GEN2
            if (usPage >= 61 && usPage <= 2291)
            {
                usLowPage = HAL_GetLowPageIndexfromHigh(usPage);
            }
            else
            {
                usLowPage = usPage - 1;
            }
#else
            usLowPage = usPage - 1;
#endif
            break;
        }
        case EXTRA_PAGE:
        {
#ifdef FLASH_IM_3DTLC_GEN2
            usHighPage = usLowPage + 1;
            usLowPage = HAL_GetLowPageIndexfromHigh(usHighPage);
#else
            usHighPage = HAL_GetHighPageIndexfromExtra(usPage);
            usLowPage = usHighPage - 1;
#endif
            break;
        }
        case LOW_PAGE_WITHOUT_HIGH:
        {
            usLowPage = usPage;
            break;
        }
        default:
        {
            break;
        }
    }

    return usLowPage;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashGetSLCPage
Description:
    get physical page according to SLC logic page.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to get physical page according to SLC logic page.
History:
    20150121    tobey   move form L3_Interface.c
------------------------------------------------------------------------------*/
U16 HAL_FlashGetSLCPage(U16 usLogicPage)
{
    return l_aSLCPageMap[usLogicPage];
}

/*------------------------------------------------------------------------------
Name: HAL_FlashRetryCheck
Description:
    check read retry flow can continue or not.
Input Param:
    U8 ucTime: the retry time want to do
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means retry can continue;
        NFC_STATUS_SUCCESS means can not.
Usage:
    called before every retry-read
History:
    20160415    abby   create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_FlashRetryCheck(U8 ucTime, BOOL bTlcMode)
{
    if (HAL_FLASH_READRETRY_CNT <= ucTime)
    {
        return NFC_STATUS_FAIL;
    }
    else
    {
        return NFC_STATUS_SUCCESS;
    }
}

/*------------------------------------------------------------------------------
Name: HAL_FlashSelRetryPara
Description:
    Select a index of retry para table
Input Param:
    pending
Output Param:
    U32 ulIndex: TLC:0~TLC_RETRY_CNT; SLC: TLC_RETRY_CNT~RETRY_CNT
Return Value:
    none
Usage:
    called when FW need change retry para by static para table
History:
    20160415    abby   create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_FlashSelRetryPara(BOOL bTlcMode)
{
    U32 ulIndex = 0; //pending

    return ulIndex;
}

/*------------------------------------------------------------------------------
Name: HAL_GetVthAdjustTime
Description:
    Get Vth adjust time by page type.
Input Param:
    U8   ucPageType;  // Low/Upp /Extra page.
    BOOL bTLCMode;
Output Param:
    ucSetTime
Return Value:

Usage:
    called when FW need change retry para by static para table
History:
    20170807    jerry   create
------------------------------------------------------------------------------*/
U8 HAL_GetVthAdjustTime(U8 ucPageType, BOOL bTLCMode)
{
    U8 ucSetTime;

    if (TRUE == bTLCMode)
    {
        if (EXTRA_PAGE == ucPageType)
        {
            ucSetTime = 4;
        }
        else if (HIGH_PAGE == ucPageType)
        {
            ucSetTime = 2;
        }
        else
        {
            ucSetTime = 1;
        }
    }
    else
    {
        ucSetTime = 1;
    }

    return ucSetTime;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashGetNewRetryParaTab
Description:
    Get a groups of retry Vth parameters
Input Param:
    U32 ulIndex
Output Param:
    none
Return Value:
    none
Usage:
    called when FW need get a new group of Vth para
History:
    20170807    jerry   create
------------------------------------------------------------------------------*/
#ifdef READ_RETRY_REFACTOR
RETRY_TABLE MCU12_DRAM_TEXT HAL_FlashGetNewRetryParaTab(U32 ulIndex, BOOL bTlcMode, U8 ucWlType, U8 ucPageType, U8 ucLevel)
{
    RETRY_TABLE tRetryPara;

    if (TRUE == bTlcMode)
    {
        if (SLC_TYPE == ucWlType)
        {
            ASSERT(ulIndex < SLC_RETRY_GROUP_CNT);
            tRetryPara = g_aSlcRetryPara[ulIndex];
        }
        else if (MLC_TYPE == ucWlType)
        {
            ASSERT(ulIndex < MLC_RETRY_GROUP_CNT);
            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
            {
                tRetryPara = g_aMlcRetryParaForLP[ulIndex];
            }
            else if (HIGH_PAGE == ucPageType)
            {
                tRetryPara = g_aMlcRetryParaForUP[ulIndex][ucLevel];
            }
            else
            {
                DBG_Printf("MLC WL page type error!\n");
                DBG_Getch();
            }
        }
        else if (TLC_TYPE == ucWlType)
        {
            ASSERT(ulIndex < TLC_RETRY_GROUP_CNT);
            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
            {
                tRetryPara = g_aTlcRetryParaForLP[ulIndex];
            }
            else if (HIGH_PAGE == ucPageType)
            {
                tRetryPara = g_aTlcRetryParaForUP[ulIndex][ucLevel];
            }
            else if (EXTRA_PAGE == ucPageType)
            {
                tRetryPara = g_aTlcRetryParaForXP[ulIndex][ucLevel];
            }
            else
            {
                DBG_Printf("TLC WL page type error!\n");
                DBG_Getch();
            }
        }
        else
        {
            DBG_Printf("WL type error!\n");
            DBG_Getch();
        }
    }
    else
    {
        ASSERT(ulIndex < SLC_RETRY_GROUP_CNT);
        tRetryPara = g_aSlcRetryPara[ulIndex];
    }

    return tRetryPara;
}
#endif

/*------------------------------------------------------------------------------
Name: HAL_FlashGetRetryParaTab
Description:
    Get a groups of retry Vth parameters
Input Param:
    U32 ulIndex
Output Param:
    none
Return Value:
    none
Usage:
    called when FW need get a new group of Vth para
History:
    20160415    abby   create
------------------------------------------------------------------------------*/
RETRY_TABLE MCU12_DRAM_TEXT HAL_FlashGetRetryParaTab(U32 ulIndex)
{
    RETRY_TABLE tRetryPara;
    tRetryPara = g_aRetryPara[ulIndex];

    return tRetryPara;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashGetMaxRetryTime
Description:
    Get max read retry time.
Input Param:
    BOOL bTLCMode
    U8 ucWLType
Output Param:

Return Value:
    ucMaxRetryTime
Usage:
    called when FW need get max read retry time
History:
    20170829    JerryNie   create
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_FlashGetMaxRetryTime(BOOL bTLCMode, U16 usPage)
{
    U8 ucMaxRetryTime;
    U8 ucWLType = HAL_GetFlashWlType(usPage, bTLCMode);

    if (TRUE == bTLCMode)
    {
        if (SLC_TYPE == ucWLType)
        {
            ucMaxRetryTime = HAL_SLC_FLASH_READRETRY_CNT;
        }
        else if (MLC_TYPE == ucWLType)
        {
            ucMaxRetryTime = HAL_MLC_FLASH_READRETRY_CNT;
        }
        else
        {
            ucMaxRetryTime = HAL_TLC_FLASH_READRETRY_CNT;
        }
    }
    else
    {
        ucMaxRetryTime = HAL_SLC_FLASH_READRETRY_CNT;
    }

    return ucMaxRetryTime;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashRetryPreConditon
Description:
    send retry pre-condition command to flash
Input Param:
    FLASH_ADDR *pFlashAddr: flash physical address
Output Param:
    none
Return Value:
    BOOL: TRUE = send command success, FALSE = fail
Usage:
    called before do read retry sequence
History:
    20160415    abby   create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashRetryPreConditon(FLASH_ADDR *pFlashAddr)
{
    return NFC_STATUS_SUCCESS;//do not need any actual precondion
}

/*------------------------------------------------------------------------------
Name: HAL_FlashRetrySendParam
Description:
    send retry command to flash
Input Param:
    FLASH_ADDR *pFlashAddr: flash physical addr
    RETRY_TABLE *pRetryPara: retry parameters, config when FW need adjust retry para,
                           otherwise it will use default value
    BOOL bRetry: TRUE: do shift read; FALSE: do normal read
    BOOL bTlcMode: TURE,TLC retry; FALSE: SLC or MLC retry
    U8 ucParaNum: setting Vth number
Output Param:
    none
Return Value:
    BOOL: TRUE = send command sucess, FALSE = fail
Usage:
    called when do read retry sequence
History:
    20160415    abby   create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashRetrySendParam(FLASH_ADDR *pFlashAddr, RETRY_TABLE *pRetryPara, BOOL bTlcMode, U8 ucParaNum)
{
    U8 ucPU, ucLun, ucStatus;
    U32 aFeature[2] = {0, 0};
    
    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    ucStatus = HAL_NfcSetFeature(pFlashAddr, pRetryPara->aRetryPara[0].bsData, pRetryPara->aRetryPara[0].bsAddr);

    //DBG_Printf("PU %d set feature ADDR 0x%x, Value 0x%x\n", ucPU, pRetryPara->aRetryPara[0].bsAddr, pRetryPara->aRetryPara[0].bsData);

//Check Feature
#if 1 
    if (NFC_STATUS_SUCCESS == HAL_NfcWaitStatus(ucPU, ucLun))
    {
        if (NFC_STATUS_FAIL == HAL_NfcGetFeature(pFlashAddr, pRetryPara->aRetryPara[0].bsAddr))
            DBG_Printf("PU %d get feature fail! ADDR 0x%x\n", ucPU, pRetryPara->aRetryPara[0].bsAddr);
        
        if (NFC_STATUS_SUCCESS == HAL_NfcWaitStatus(ucPU, ucLun))
        {
            HAL_DecSramGetFlashId(pFlashAddr, aFeature);
            //DBG_Printf("PU %d get feature success! ADDR 0x%x value 0x%x\n", ucPU, pRetryPara->aRetryPara[0].bsAddr, aFeature[0]);
        }
        else
            DBG_Printf("PU %d get feature fail! ADDR 0x%x value 0x%x\n", ucPU, pRetryPara->aRetryPara[0].bsAddr, aFeature[0]);
    }
#endif

    return ucStatus;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashRetryEn
Description:
    send retry pre-condition command to flash
Input Param:
    FLASH_ADDR *pFlashAddr: flash addr
    BOOL bRetry: TRUE: do shift read; FALSE: do normal read
Output Param:
    none
Return Value:
    BOOL: TRUE = send command success, FALSE = fail
Usage:
    called before read a page after adjust Vth in shift read sequence
History
    20160116    abby    create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashRetryEn(FLASH_ADDR *pFlashAddr, BOOL bRetry)
{
    return  NFC_STATUS_SUCCESS;
}



/*------------------------------------------------------------------------------
Name: HAL_FlashRetryTerminate
Description:
    terminate flash read retry sequence
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    BOOL: TRUE = send command sucess, FALSE = fail
Usage:
    called when read retry sequence finish
History:
    20140911    Gavin   modify to meet coding style
    20151123    abby    rename from HAL_FlashReadRetryTerminate
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashRetryTerminate(U8 ucPU, U8 ucLun, BOOL bTlcMode)
{
    FLASH_ADDR tFlashAddr = {0};

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = ucLun;

    while (NFC_STATUS_SUCCESS != HAL_NfcSetFeature(&tFlashAddr, 0, 0x89));

    return NFC_STATUS_SUCCESS;
}

#ifdef READ_RETRY_REFACTOR
/*------------------------------------------------------------------------------
Name: HAL_FlashHomemadeVtTerminate
Description:
    terminate flash homemade Vt read retry sequence
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    BOOL: TRUE = send command sucess, FALSE = fail
Usage:
    When a homemade read retry entry is uesd, it must be called after a read retry, regardless of success or failure
History:
    20170828    JerryNie   create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashHomemadeVtTerminate(U8 ucPU, U8 ucLun, BOOL bTlcMode, U8 ucWlType, U8 ucPageType)
{
    RETRY_TABLE tRetryPara;
    FLASH_ADDR tFlashAddr = { 0 };
    U8 ucLevel, ucVthAdjustTime, ucFeatureAddr;

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = ucLun;

    ucVthAdjustTime = HAL_GetVthAdjustTime(ucPageType, bTlcMode);
    for (ucLevel = 0; ucLevel < ucVthAdjustTime; ucLevel++)
    {
        tRetryPara = HAL_FlashGetNewRetryParaTab(0, bTlcMode, ucWlType, ucPageType, ucLevel);
        ucFeatureAddr = tRetryPara.aRetryPara[0].bsAddr;
        HAL_NfcSetFeature(&tFlashAddr, 0, ucFeatureAddr);

        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
        {
            DBG_Printf("Pu%d Lun%d Terminate Wait Fail.\n", ucPU, ucLun);
            DBG_Getch();
        }
    }

    return NFC_STATUS_SUCCESS;
}
#endif

#ifndef SIM
U32 HAL_FlashRetryCheckForPart0(U8 ucTime)
{
    if (HAL_FLASH_READRETRY_CNT <= ucTime)
    {
        return NFC_STATUS_FAIL;
    }
    else
    {
        return NFC_STATUS_SUCCESS;
    }
}

U32 HAL_FlashSelRetryParaForPart0(void)
{
    U32 ulIndex = 0; //pending

    return ulIndex;
}

RETRY_TABLE HAL_FlashGetRetryParaTabForPart0(U32 ulIndex)
{
    RETRY_TABLE tRetryPara;
    tRetryPara = g_aRetryParaForPart0[ulIndex];

    return tRetryPara;
}

BOOL HAL_FlashRetryPreConditonForPart0(U8 ucPU)
{
    return NFC_STATUS_SUCCESS;//do not need any actual precondion
}

BOOL HAL_FlashRetrySendParamForPart0(U8 ucPU, RETRY_TABLE *pRetryPara, U8 ucParaNum)
{
    if (FALSE == BL_Part0IsLunAccessable(ucPU))
        return NFC_STATUS_FAIL;

    BL_Part0SetFeature(ucPU, pRetryPara->aRetryPara[0].bsData, pRetryPara->aRetryPara[0].bsAddr);

    return NFC_STATUS_SUCCESS;
}

BOOL HAL_FlashRetryEnForPart0(U8 ucPU, BOOL bRetry)
{
    return  NFC_STATUS_SUCCESS;
}

BOOL HAL_FlashRetryTerminateForPart0(U8 ucPU)
{
    BL_Part0SetFeature(ucPU, 0, 0x89);
    return NFC_STATUS_SUCCESS;
}
#endif


/*------------------------------------------------------------------------------
Name: HAL_FlashGetPlnNumFromNfcq
Description:
    get PlnNum form anlysis NFCQ RowAddr.
Input Param:
    NFCQ_ENTRY* pNFCQEntry: pointer to nfcq entry.
Output Param:
    none
Return Value:
    U8: Pln number
Usage:
    FW call this function to get PlnNum form anlysis NFCQ RowAddr, when ReadRetry happen.
History:
    20160416    abby    create
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_FlashGetPlnNumFromNfcq(NFCQ_ENTRY * pNfcqEntry)
{
    U8 ucPhyPln, ucPln;
    U16 usPage;

    usPage = pNfcqEntry->atRowAddr[0].bsRowAddr & ((1<<PG_PER_BLK_BITS)-1);

    ucPhyPln = (pNfcqEntry->atRowAddr[0].bsRowAddr >> PG_PER_BLK_BITS) & PLN_PER_LUN_MSK;

    ucPln = (1 != usPage % 2) ? ucPhyPln : (ucPhyPln + PLN_PER_LUN);

    return ucPln;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashIs1PlnOp
Description:
    To detect if current req is single plane operation
Input Param:
    U8 ucCurrCmdType: cmd type.
Output Param:
    none
Return Value:
    none
Usage:
    In FW. Called when need detect if the req is single plane operation
History:
    20160215    abby    create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashIs1PlnOp(U8 ucReqType)
{
    BOOL ret = FALSE;

    /*SLC mode*/
    if ((NF_PRCQ_READ == ucReqType)||(NF_PRCQ_PRG == ucReqType)
       ||(NF_PRCQ_PRG_LOW_PG == ucReqType))
    {
        ret = TRUE;
    }

    /*TLC mode*/
    if ((NF_PRCQ_TLC_READ == ucReqType) || (NF_PRCQ_TLC_PRG == ucReqType)
        ||(NF_PRCQ_TLC_PRG_LOW_PG == ucReqType))
    {
        ret = TRUE;
    }

    return ret;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashIsTrigCacheOp
Description:
    This bit indicates the current request could be issued as a read/write cache operation
Input Param:
    U8 ucCurrCmdType: cmd type.
Output Param:
    none
Return Value:
    none
Usage:
    --
History:
    20140909    Tobey   moved from HAL_FlashDriver.c
    20160531    abby    move from HAL_FlashBasicDriver.c,
                        rename from HalNfcIsTrigCacheOp, add 1 plane cache
------------------------------------------------------------------------------*/
BOOL HAL_FlashIsTrigCacheOp(U8 ucCurrCmdType)
{
    BOOL ret = FALSE;

#if (!defined(BOOTLOADER) && defined(FLASH_CACHE_OPERATION))
    /*SLC mode*/
    if ((NF_PRCQ_READ_MULTIPLN == ucCurrCmdType)||(NF_PRCQ_PRG_MULTIPLN == ucCurrCmdType)
    ||(NF_PRCQ_PRG_MULTIPLN_LOW_PG_NO_HIGH == ucCurrCmdType)||(NF_PRCQ_PRG_MULTIPLN_LOW_PG == ucCurrCmdType))
    {
        ret = TRUE;
    }
    /*TLC mode*/
    if ((NF_PRCQ_TLC_READ_MULTIPLN == ucCurrCmdType) || (NF_PRCQ_TLC_PRG_MULTIPLN == ucCurrCmdType)
    || (NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG_NO_HIGH == ucCurrCmdType) || (NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG == ucCurrCmdType))
    {
        ret = TRUE;
    }
    else// 1 plane cache
    {
        ret = HAL_FlashIs1PlnOp(ucCurrCmdType);
    }
#endif 

    return ret;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashIsBlockGood
Description:
    check if one special block bad or not.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: FALSE: good block; TRUE: bad block.
Usage:
History:
    20150121    tobey   move form HAL_FlashDriverExt.c
    20150604    jason   update
    20151221    abby    replace bytemode read by normal read
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashIsBlockBad(U8 ucPu, U8 ucLun, U8 ucPlane, U16 usBlk)
{
    U32 ulByteOffsetOfMarkCw, ulByteOffsetInCw, ulByteOffsetInBuf, ulBufAddr;
    BOOL bBadBlk;
    U8 ucCw;
    FLASH_ADDR PhyAddr = {0};
    NFC_READ_REQ_DES tRdReq ={0};
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;

    /*1. Calculation which CW the bad blk mark is in */
    ucCw = BAD_BLK_MARK_COLUMN_POS / (CW_INFO_SZ + LDPC_MAT_PRT_LEN_FST_15K);//14

    /*2. Calculation the offset of marked Cw in page */
    ulByteOffsetOfMarkCw = (CW_INFO_SZ + LDPC_MAT_PRT_LEN_FST_15K) * ucCw;
    
    if (pDsReg->atDSEntry[DS_ENTRY_SEL].bsNCRCEn)
    {
        ulByteOffsetOfMarkCw = ulByteOffsetOfMarkCw + 2 * ucCw;
    }
    if (pDsReg->atDSEntry[DS_ENTRY_SEL].bsLbaEn)
    {
        ulByteOffsetOfMarkCw = ulByteOffsetOfMarkCw + 4 * (ucCw * CW_INFO_SZ / LPN_SIZE);
    }

    /*3. Calculation the offset of bad block mark positon in last marked Cw*/
    ulByteOffsetInCw = BAD_BLK_MARK_COLUMN_POS - ulByteOffsetOfMarkCw;

    PhyAddr.ucPU = ucPu;
    PhyAddr.ucLun = ucLun;
    PhyAddr.bsPln = ucPlane;
    PhyAddr.usBlock = usBlk;
    PhyAddr.usPage = 0;

    /*    calculate sector addr by column addr, only read 1 CW  */
    tRdReq.bsSecStart = 0;//ulByteLen / SEC_SIZE;
    tRdReq.bsSecLen   = SEC_PER_LOGIC_PG;
        
    /*    allocate buffer ID and get DRAM address of read data    */
    tRdReq.bsRdBuffId  = ((DRAM_DATA_BUFF_MCU1_BASE - DRAM_START_ADDRESS) / PHYPG_SZ) + 10;
    ulBufAddr = (HAL_NfcGetDmaAddr(tRdReq.bsRdBuffId, 0, PHYPG_SZ_BITS) << 1) + DRAM_START_ADDRESS ;
  
    tRdReq.ppNfcRed = NULL;
    tRdReq.bsRawData = TRUE;  //need read raw data to avoid ECC engine change or miss the IDB info
    tRdReq.pErrInj = NULL;
    /*  scramble disable    */
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
    pNfcPgCfg->bsScrBps = TRUE;

    /*4. read 1 plane data   */
    while (TRUE == HAL_NfcGetFull(ucPu, ucLun));
    
    HAL_NfcSinglePlnRead(&PhyAddr, &tRdReq, FALSE);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPu, ucLun))
    {
        DBG_Printf("Pu%d Lun%d Pln%d Blk%d Page%d Read IDB was rejected.\n", ucPu, ucLun, ucPlane, usBlk, PhyAddr.usPage);
        //DBG_Getch();
    }

    /*5. Calculation the location of bad block mark in buffer*/
    ulByteOffsetInBuf = ulByteOffsetInCw + CW_INFO_SZ * 2 * ucCw + ulBufAddr - 1;

#if 0
    DBG_Printf("PU%d BLK%d Pln%d buffAddr0x%x_0x%x:\n",ucPu,usBlk,ucPlane,ulBufAddr,ulByteOffsetInBuf);
    DBG_Printf("*(IDb-1)=0x%x\n", *(U8 *)(ulByteOffsetInBuf - 1));
    DBG_Printf("*(IDb)=0x%x\n", *(U8 *)(ulByteOffsetInBuf));
    DBG_Printf("*(IDb+1)=0x%x\n", *(U8 *)(ulByteOffsetInBuf + 1));
#endif

    if (BAD_BLK_IDB_MARK == (*(U8 *)ulByteOffsetInBuf & 0xFF))
    {
        bBadBlk = TRUE;
    }
    else
    {
        bBadBlk = FALSE;
    }

    return bBadBlk;
}

/*-----------------------------------------------------------------------------
 Prototype      : HAL_IsFlashPageSafed
 Description    : to check if the flash page is safed for L95 issue.
 Input          : U32 ulCE
 Output         : None
 Return Value   : U32
 Calls          :
 Called By      :
20160415    abby    create
-----------------------------------------------------------------------------*/
U32 HAL_IsFlashPageSafed(U8 ucPu, U16 usWritePPO, U16 usReadPPO)//pending
{
    U16 usSafeDelta;
    if (usWritePPO < usReadPPO)
    {
        DBG_Printf("MCU#%d Flash Page isn't safe. usWritePPO = %d, usReadPPO = %d, getch\n", HAL_GetMcuId(), usWritePPO, usReadPPO);
        DBG_Getch();
    }

    /* for Intel 3D TLC, the max safe interval between low page and extra page in a WL up to 96 */
    usSafeDelta = usWritePPO - usReadPPO;
    if (usSafeDelta < PAIR_PAGE_INTERVAL_MAX)
    {
        //DBG_Printf("MCU#%d Flash Page isn't safe. usWritePPO = %d, usReadPPO = %d\n", HAL_GetMcuId(), usWritePPO, usReadPPO);
        return FALSE;
    }

    return TRUE;
}

// not support tlc prg cycle and wl. only tmp for compile
U8  HAL_FlashGetTlcPrgCycle(U16 usPrgIndex)
{
    return 0;
}
U16 HAL_FlashGetTlcPrgWL(U16 usPrgIndex)
{
    return usPrgIndex;
}

U8 HAL_FlashGetScrSeed(U16 usPage, U8 ucScrMod, U8 ucPageType)
{
    return usPage & MSK_7F;
}

/*   end of this file   */
