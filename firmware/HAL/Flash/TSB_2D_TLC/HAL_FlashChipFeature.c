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
20160108    abby    modify to meet VT3533
*******************************************************************************/
#include "HAL_FlashDriverBasic.h"
#include "HAL_LdpcEngine.h"
#include "HAL_HostInterface.h"
#ifdef SIM
#include "sim_flash_shedule.h"
#endif

//static retry para table, actural value pending for more test data
LOCAL MCU12_DRAM_TEXT RETRY_TABLE g_aRetryPara[RETRY_CNT] =
{
    /*  TLC  */
    /* TLC_RETRY_PARA_GROUP0 */
    {
        0x55, 0x04, 0x4,  INVALID_2F,
        0x55, 0x05, 0x4,  INVALID_2F,
        0x55, 0x06, 0x4,  INVALID_2F,
        0x55, 0x07, 0x4,  INVALID_2F,
        0x55, 0x08, 0x4,  INVALID_2F,
        0x55, 0x09, 0x4,  INVALID_2F,
        0x55, 0x0A, 0x4,  INVALID_2F
    },
    /* TLC_RETRY_PARA_GROUP1 */    
    {
        0x55, 0x04, 0xF9, INVALID_2F,
        0x55, 0x05, 0xFA, INVALID_2F,
        0x55, 0x06, 0xFC, INVALID_2F,
        0x55, 0x07, 0xFC, INVALID_2F,
        0x55, 0x08, 0xFC, INVALID_2F,
        0x55, 0x09, 0xFA, INVALID_2F,
        0x55, 0x0A, 0xFC, INVALID_2F
    },
    /* TLC_RETRY_PARA_GROUP2 */
    {
        0x55, 0x04, 0xF8, INVALID_2F,
        0x55, 0x05, 0xF9, INVALID_2F,
        0x55, 0x06, 0xFB, INVALID_2F,
        0x55, 0x07, 0xFB, INVALID_2F,
        0x55, 0x08, 0xFA, INVALID_2F,
        0x55, 0x09, 0xF8, INVALID_2F,
        0x55, 0x0A, 0xFA, INVALID_2F
    },
    /* TLC_RETRY_PARA_GROUP3 */
    {
        0x55, 0x04, 0xF7, INVALID_2F,
        0x55, 0x05, 0xF8, INVALID_2F,
        0x55, 0x06, 0xFA, INVALID_2F,
        0x55, 0x07, 0xF9, INVALID_2F,
        0x55, 0x08, 0xF8, INVALID_2F,
        0x55, 0x09, 0xF6, INVALID_2F,
        0x55, 0x0A, 0xF8, INVALID_2F
    },
    /* TLC_RETRY_PARA_GROUP4 */
    {
        0x55, 0x04, 0xF6, INVALID_2F,
        0x55, 0x05, 0xF7, INVALID_2F,
        0x55, 0x06, 0xF9, INVALID_2F,
        0x55, 0x07, 0xF7, INVALID_2F,
        0x55, 0x08, 0xF6, INVALID_2F,
        0x55, 0x09, 0xF4, INVALID_2F,
        0x55, 0x0A, 0xF5, INVALID_2F
    },
    /* TLC_RETRY_PARA_GROUP5 */
    {
        0x55, 0x04, 0xF5, INVALID_2F,
        0x55, 0x05, 0xF6, INVALID_2F,
        0x55, 0x06, 0xF8, INVALID_2F,
        0x55, 0x07, 0xF6, INVALID_2F,
        0x55, 0x08, 0xF4, INVALID_2F,
        0x55, 0x09, 0xF2, INVALID_2F,
        0x55, 0x0A, 0xF3, INVALID_2F
    },
    /*  SLC  */
    /* RETRY_PARA_GROUP0 */
    {
        0x55, 0x0B, 0xF0, INVALID_2F,
        0x55, 0x0D, 0x0,  INVALID_2F,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    },
    /* RETRY_PARA_GROUP1 */
    {
        0x55, 0x0B, 0xE0, INVALID_2F,
        0x55, 0x0D, 0x0,  INVALID_2F,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    },
    /* RETRY_PARA_GROUP2 */
    {
        0x55, 0x0B, 0xD0, INVALID_2F,
        0x55, 0x0D, 0x0, INVALID_2F, 
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    },
    /* RETRY_PARA_GROUP3 */
    {
        0x55, 0x0B, 0xC0, INVALID_2F,
        0x55, 0x0D, 0x0,  INVALID_2F,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    },
    /* RETRY_PARA_GROUP4 */
    {
        0x55, 0x0B, 0x20, INVALID_2F,
        0x55, 0x0D, 0x0,  INVALID_2F,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    },
    /* RETRY_PARA_GROUP5 */
    {
        0x55, 0x0B, 0x30, INVALID_2F,
        0x55, 0x0D, 0x0,  INVALID_2F,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    },
    /* RETRY_PARA_GROUP6 */
    {
        0x55, 0x0B, 0x40, INVALID_2F,
        0x55, 0x0D, 0x0,  INVALID_2F,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    }
};

/*------------------------------------------------------------------------------
    LOCAL FUNCTION DECLARATION 
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HAL_FlashUpdateRetryParaPrcq(U8 ucReqType, RETRY_TABLE *pRetryPara, U8 ucParaNum);

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
    20141024    Gavin   create. merge feature initialization to this function
    20151221    abby    modify to meet VT3533
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
        /* TOGGLE flash need set to 1    */
        pNfcPgCfg->bsMulLunToggle = TRUE;
        /* LUN bonding choose, TSB need set 1, L95 set 0  */
        pNfcPgCfg->bsLunBdEn = TRUE;
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
    20150121    tobey   move form L3_Interface.c
    20150210    abby    TLC do not need SLC mapping
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashInitSLCMapping(void)
{
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetFlashPairPageType
Description:
    Get MLC page type in a pair of page.
Input Param:
    usPage: physical page
Output Param:
    none
Return Value:
    none
Usage:
    called in TLC winsim, only for compile
History:
    20150121    abby    TLC only for compile in winsim
------------------------------------------------------------------------------*/
PAIR_PAGE_TYPE HAL_GetFlashPairPageType(U16 usPage)
{
    PAIR_PAGE_TYPE ePairPageType = LOW_PAGE;

    return ePairPageType;
}

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
------------------------------------------------------------------------------*/
U16 HAL_GetLowPageIndex(U16 usHighPage)
{
    return usHighPage;
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
    return usLogicPage;
}

#ifndef BOOTLOADER
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
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
    20151123    abby    rename from HAL_FlashReadRetryCheck
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_FlashRetryCheck(U8 ucTime, BOOL bTlcMode)
{
    U8 ucRetryMaxTime;
    ucRetryMaxTime = (FALSE == bTlcMode) ? HAL_FLASH_READRETRY_CNT : HAL_TLC_FLASH_READRETRY_CNT;

    if (ucRetryMaxTime <= ucTime)
    {
        return NFC_STATUS_FAIL;
    }
    else
    {
        return NFC_STATUS_SUCCESS;
    }
}

/*------------------------------------------------------------------------------
Name: HAL_FlashUpdateRetryPrcq
Description: 
    send retry command to flash
Input Param:
    U8 ucReqType:cmd type;
    RETRY_TABLE *pRetryPara: retry parameters, config when FW need adjust retry para,
                           otherwise it will use default value
     U8 ucParaNum: parameters setting number
Output Param:
    none
Return Value:
    none
Usage:
    called when FW need rewrite retry PRCQ: cmd/addr/Vt/para_number can be adjusted in the run-time
    only for TSB retry sequence
History:
    20160115    abby    create, index pending
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HAL_FlashUpdateRetryParaPrcq(U8 ucReqType, RETRY_TABLE *pRetryPara, U8 ucParaNum)                            
{
    U32 *pPrcqEntry;
    U8 ucParaIndex;
       
    /*    rewrite PRCQ table    */
    pPrcqEntry =(U32 *) HAL_NfcGetPrcqEntryAddr(ucReqType);
    
    for (ucParaIndex = 0; ucParaIndex < ucParaNum; ucParaIndex++)
    {
        *pPrcqEntry++ = RAW_PIO_CMD | PIO_CFG_VALUE(pRetryPara->aRetryPara[ucParaIndex].bsCmd);
        *pPrcqEntry++ = RAW_PIO_ADDR | PIO_CFG_VALUE(pRetryPara->aRetryPara[ucParaIndex].bsAddr);
        if (ucParaIndex == ucParaNum - 1)//last parameter
        {
            *pPrcqEntry++ = RAW_PIO_IO_LAST_PHS(1) | RAW_PIO_DATAOUT | PIO_CFG_VALUE(pRetryPara->aRetryPara[ucParaIndex].bsData);
        }
        else
        {
            *pPrcqEntry++ = RAW_PIO_DATAOUT | PIO_CFG_VALUE(pRetryPara->aRetryPara[ucParaIndex].bsData);
        }
    }

    return;
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
    20160115    abby    create, pending for FW policy
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_FlashSelRetryPara(BOOL bTlcMode)                            
{
    U32 ulIndex;
    ulIndex = (FALSE != bTlcMode) ? 0 : (TLC_RETRY_CNT + 0);//0 is default, pending

    return ulIndex;
}

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
    20160115    abby    create
------------------------------------------------------------------------------*/
RETRY_TABLE MCU12_DRAM_TEXT HAL_FlashGetRetryParaTab(U32 ulIndex)                            
{
    RETRY_TABLE tRetryPara;
    tRetryPara = g_aRetryPara[ulIndex];
    
    return tRetryPara;
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
    20140911    Gavin   modify to meet coding style
    20160116    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashRetryPreConditon(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY * pNFCQEntry;
    U8 ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
           
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_RETRY_PRE);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_RETRY_PRE, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
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
    20140911    Gavin   modify to meet coding style
    20160115    abby    modify to 533, provides flexible interfaces
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashRetrySendParam(FLASH_ADDR *pFlashAddr, RETRY_TABLE *pRetryPara, BOOL bTlcMode, U8 ucParaNum)                            
{
    U8 ucPU, ucLun, ucCH;
    NFCQ_ENTRY * pNFCQEntry;
    U8 ucReqType;
    
    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucCH  = HAL_NfcGetPhyPU(pFlashAddr->ucPU) % NFC_CH_TOTAL;
    
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (FALSE == bTlcMode)//SLC or MLC retry
    {
        ucReqType = NF_PRCQ_RETRY_ADJ0 + ucCH;
    }
    else//TLC retry
    {
        ucReqType = NF_PRCQ_TLC_RETRY_ADJ0 + ucCH;
    }

    /*    rewrite PRCQ table when para need to adjust    */
    if (NULL != pRetryPara)
    {
        HAL_FlashUpdateRetryParaPrcq(ucReqType, pRetryPara, ucParaNum);
    }

    /*    fill NFCQ    */
    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);
    HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
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
    NFCQ_ENTRY * pNFCQEntry;
    U8 ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }
    if (FALSE == bRetry)
    {
        return NFC_STATUS_SUCCESS;
    }
    
    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_RETRY_EN);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_RETRY_EN, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
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

    #ifdef DUAL_LUN_PER_PU
    if (NFC_STATUS_SUCCESS != HAL_NfcResetLun(&tFlashAddr))
    #else
    if (NFC_STATUS_SUCCESS != HAL_NfcResetFlash(&tFlashAddr))
    #endif
    {
        DBG_Printf("HAL_FlashRetryTerminate send fail. pu=%d\n", ucPU);
    }

    return NFC_STATUS_SUCCESS;
}

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
    20141121    Tobey   create
    20151119    abby    modify PLN_PER_LUN_BITS to PHY_PG_PER_BLK_MSK to fix bug
                        change name from HalNfcGetPlnNumFromNfcq
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_FlashGetPlnNumFromNfcq(NFCQ_ENTRY * pNfcqEntry)
{
    U8 ucPln;
    ucPln = (pNfcqEntry->atRowAddr[0].bsRowAddr >> PLN_POS_IN_ROW_ADDR) & PLN_PER_LUN_MSK;

    return ucPln;
}
#endif

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

    if ((NF_PRCQ_READ == ucReqType) || (NF_PRCQ_PRG == ucReqType)
        || (NF_PRCQ_TLC_PRG_1ST == ucReqType) || (NF_PRCQ_TLC_PRG_2ND == ucReqType)
        || (NF_PRCQ_TLC_PRG_3RD == ucReqType) || (NF_PRCQ_TLC_READ_LP == ucReqType)
        || (NF_PRCQ_TLC_READ_MP == ucReqType) || (NF_PRCQ_TLC_READ_UP == ucReqType))
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
    if ((NF_PRCQ_READ_MULTIPLN == ucCurrCmdType)||(NF_PRCQ_PRG_MULTIPLN == ucCurrCmdType)
        ||(NF_PRCQ_TLC_READ_LP_MULTIPLN == ucCurrCmdType)||(NF_PRCQ_TLC_READ_MP_MULTIPLN == ucCurrCmdType)
        ||(NF_PRCQ_TLC_READ_UP_MULTIPLN == ucCurrCmdType)||(NF_PRCQ_TLC_PRG_1ST_MULTIPLN == ucCurrCmdType)
    ||(NF_PRCQ_TLC_PRG_2ND_MULTIPLN == ucCurrCmdType)||(NF_PRCQ_TLC_PRG_3RD_MULTIPLN == ucCurrCmdType)
    ||(NF_PRCQ_TLC_PRG_1ST_LP_MULTIPLN == ucCurrCmdType)||(NF_PRCQ_TLC_PRG_1ST_MP_MULTIPLN == ucCurrCmdType)
    ||(NF_PRCQ_TLC_PRG_1ST_UP_MULTIPLN == ucCurrCmdType))
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
    U8  ucRdCnt;  
    U32 ulByteLen;      //14K info lenth
    U32 ulByteOffset;   //bad blk mark offset in 15 CW
    U32 ulDataAddr;
    BOOL bFlashStatus;
    BOOL bBadBlk;
    FLASH_ADDR PhyAddr = {0};
    NFC_READ_REQ_DES tRdReq ={0};
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    
    /*  Calculation bad blk mark location in MSG CW   */   
    ulByteLen = (CW_INFO_SZ + LDPC_MAT_PRT_LEN_FST_15K) * 14;
    if (pDsReg->atDSEntry[DS_ENTRY_SEL].bsNCRCEn)
    {
        ulByteLen = ulByteLen + 2*14;
    }
    if (pDsReg->atDSEntry[DS_ENTRY_SEL].bsLbaEn)
    {
        ulByteLen = ulByteLen + 4*3;
    }
    ulByteOffset = BAD_BLK_MARK_COLUMN_POS - ulByteLen;
    
    for(ucRdCnt = 0;ucRdCnt < 2;ucRdCnt++)
    {     
        PhyAddr.ucPU = ucPu;
        PhyAddr.ucLun = ucLun;
        PhyAddr.bsPln = ucPlane;
        PhyAddr.usBlock = usBlk;
        PhyAddr.usPage = ucRdCnt*(PG_PER_BLK - 1); // the first page and the last page

        /*    calculate sector addr by column addr, only read 15 CW  */
        tRdReq.bsSecStart = ulByteLen / SEC_SIZE;
        tRdReq.bsSecLen   = 2;
        
        /*    allocate buffer ID and get DRAM address of read data    */
        tRdReq.bsRdBuffId = (DRAM_DATA_BUFF_MCU1_BASE - DRAM_START_ADDRESS) / LOGIC_PG_SZ + ucRdCnt;
        ulDataAddr = HAL_NfcGetDmaAddr(tRdReq.bsRdBuffId, tRdReq.bsSecStart, PHYPG_SZ_BITS) + DRAM_START_ADDRESS;
        tRdReq.ppNfcRed = NULL;
        tRdReq.bsRawData = TRUE;  //need read raw data to avoid ECC engine change or miss the IDB info
        tRdReq.pErrInj = NULL;

        bFlashStatus = HAL_NfcSinglePlnRead(&PhyAddr, &tRdReq, FALSE);
        if (NFC_STATUS_SUCCESS != bFlashStatus)
        {
            DBG_Printf("Pu%d Lun%d Pln%d Blk%d Page%d Read IDB was rejected.\n", ucPu, ucLun, ucPlane, usBlk, PhyAddr.usPage);
            DBG_Getch();
        }

        bFlashStatus = HAL_NfcWaitStatus(ucPu, ucLun);
        if(NF_SUCCESS != bFlashStatus)
        {
            HAL_NfcResetCmdQue(ucPu, ucLun);
            HAL_NfcClearINTSts(ucPu, ucLun);
            bBadBlk = TRUE;
            break;
        }
        else if (NORMAL_BLK_MARK != (((U8)ulDataAddr + ulByteOffset) & 0xFF))
        {
            bBadBlk = TRUE;
            break;
        }
        else
        {
            bBadBlk = FALSE;
        }
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

 History        :
 1.Date         : 2014/4/2
   Author       : henryluo
   Modification : Created function
 2.Date         : 2014/11/28
   Author       : JasonGuo
   Modification : Modify
 3.Data         : 2015/7/9
   Author       : JasonGuo
   Modification : according abblin's solution to modify it. TSB-15nmChip, need to meet the share pages rules.
   20160117        abby    For TLC is useless,just return
-----------------------------------------------------------------------------*/
U32 HAL_IsFlashPageSafed(U8 ucPu, U16 usWritePPO, U16 usReadPPO)
{
    return TRUE;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashGetTlcPrgWL
Description: 
    calculate WL number from program order index.
Input Param:
    U16 usPrgIndex: program WL index
Output Param:
    none
Return Value:
    U8 ucWL: WL number in program operation
Usage: 
    call when program TLC block in special order
History:
    20160206     abby      add header
------------------------------------------------------------------------------*/
U16 HAL_FlashGetTlcPrgWL(U16 usPrgIndex)
{
    U16 usWl = 0;
    U16 usMaxPageNum = (PGADDR_PER_PRG * INTRPG_PER_PGADDR * PG_PER_BLK) - 1;
    U8 ucMaxWlNum = PG_PER_BLK - 1;
    U8 ucRem, ucQuo;

    if(0 == usPrgIndex || 2 == usPrgIndex)
    {
        usWl = 0;
        return usWl;
    }
    if(1 == usPrgIndex)
    {
        usWl = 1;
        return usWl;
    }
    if((usMaxPageNum - 1) == usPrgIndex)
    {
        usWl = ucMaxWlNum - 1;
        return usWl;
    }
    if(((usMaxPageNum - 2) == usPrgIndex) || (usMaxPageNum == usPrgIndex))
    {
        usWl = ucMaxWlNum;
        return usWl;
    }
    if(usPrgIndex > 2 && usPrgIndex < (usMaxPageNum - 2))
    {
        ucQuo = usPrgIndex / 3;
        ucRem = usPrgIndex % 3;
        if(0 == ucRem)
        {
            usWl = ucQuo + 1;
        }
        else if(1 == ucRem)
        {
            usWl = ucQuo;
        }
        else
        {
            usWl = ucQuo - 1;
        }
        return usWl;
    }

    return usWl;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashGetTlcPrgCycle
Description: 
    calculate program time in current WL.
Input Param:
    U16 usPrgIndex: program WL index
Output Param:
    none
Return Value:
    U8 ucPrgCycle: 0-2
Usage: 
    call when program TLC block in special order
History:
    20160206     abby      add header
------------------------------------------------------------------------------*/
U8 HAL_FlashGetTlcPrgCycle(U16 usPrgIndex)
{
    U8 ucPrgCycle = INVALID_2F;
    U16 usMaxPageNum = (PGADDR_PER_PRG * INTRPG_PER_PGADDR * PG_PER_BLK) - 1;

    if(0 == usPrgIndex || 1 == usPrgIndex)
    {
        ucPrgCycle = 0;
        return ucPrgCycle;
    }
    if(2 == usPrgIndex || (usMaxPageNum - 2) == usPrgIndex)
    {
        ucPrgCycle = 1;
        return ucPrgCycle;
    }
    if(usMaxPageNum == usPrgIndex || (usMaxPageNum - 1) == usPrgIndex)
    {
        ucPrgCycle = 2;
        return ucPrgCycle;
    }
    if(usPrgIndex > 2 && usPrgIndex < (usMaxPageNum - 2))
    {
        ucPrgCycle = usPrgIndex%3;
        return ucPrgCycle;
    }
    
    return ucPrgCycle;
}

/*     end of this file    */
