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

LOCAL U16  l_aSLCPageMap[PG_PER_BLK];

//static retry para table, actural value pending for more test data
LOCAL MCU12_DRAM_TEXT RETRY_TABLE g_aRetryPara[RETRY_CNT] =
{
    /* YMTC 3D MLC:SET FEATURE */
    {   0xEF, 0x89, 0x0, INVALID_2F     },
    {   0xEF, 0x89, 0x1, INVALID_2F     },
    {   0xEF, 0x89, 0x2, INVALID_2F     },
    {   0xEF, 0x89, 0x3, INVALID_2F     },
    {   0xEF, 0x89, 0x4, INVALID_2F     }
};

#ifndef SIM
//Henry add. For BL Part0 retry. (Must be located in OTFB)
RETRY_TABLE g_aRetryParaForPart0[RETRY_CNT] =
{
    /* YMTC 3D MLC:SET FEATURE */
    {   0xEF, 0x89, 0x0, INVALID_2F     },
    {   0xEF, 0x89, 0x1, INVALID_2F     },
    {   0xEF, 0x89, 0x2, INVALID_2F     },
    {   0xEF, 0x89, 0x3, INVALID_2F     },
    {   0xEF, 0x89, 0x4, INVALID_2F     }
};
#endif

/*------------------------------------------------------------------------------
    LOCAL FUNCTION DECLARATION 
------------------------------------------------------------------------------*/

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
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashInitSLCMapping(void)
{
    U32 ulLogicPage;
    
    for(ulLogicPage = 0; ulLogicPage < PG_PER_BLK; ulLogicPage++)
    {
        l_aSLCPageMap[ulLogicPage] = ulLogicPage;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetFlashPairPageType
Description:
    Get MLC page type in a pair of page.
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
------------------------------------------------------------------------------*/
PAIR_PAGE_TYPE HAL_GetFlashPairPageType(U16 usPage)
{
    PAIR_PAGE_TYPE ePairPageType;

	if (0 == usPage/2)
		ePairPageType = LOW_PAGE;
	else
		ePairPageType = HIGH_PAGE;

    return ePairPageType;
}

WL_TYPE HAL_GetFlashWlType(U16 usPage, BOOL bsTLC)
{
    U8 ucWLType = MLC_TYPE;


    return ucWLType;
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
    U16 usLowPage = usHighPage - 1;

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
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
    20151123    abby    rename from HAL_FlashReadRetryCheck
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
    20160115    abby    create, pending for FW policy
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
    U8 ucSetTime = 1;

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
    return NFC_STATUS_SUCCESS;//L85 flash do not need any actual precondion
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
    20141121    Tobey   create
    20151119    abby    modify PLN_PER_LUN_BITS to PHY_PG_PER_BLK_MSK to fix bug
                        change name from HalNfcGetPlnNumFromNfcq
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_FlashGetPlnNumFromNfcq(NFCQ_ENTRY * pNfcqEntry)
{
    U8 ucPln;
    ucPln = 0;

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

    if ((NF_PRCQ_READ == ucReqType)||(NF_PRCQ_PRG == ucReqType) ||(NF_PRCQ_MLC_READ == ucReqType)||(NF_PRCQ_MLC_PRG == ucReqType))
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
    if ((NF_PRCQ_READ_FULL_PLN == ucCurrCmdType)||(NF_PRCQ_PRG_FULL_PLN == ucCurrCmdType))
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
    20151221    abby    replace bytemode read by normal read, 16384 byte in 15K CW in 533
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
    if (0xFF != (*(U8 *)ulByteOffsetInBuf & 0xFF))
    {
        DBG_Printf("PU%d BLK%d Pln%d buffAddr0x%x_0x%x:\n",ucPu,usBlk,ucPlane,ulBufAddr,ulByteOffsetInBuf);
        //DBG_Printf("*(IDb-1)=0x%x\n", *(U8 *)(ulByteOffsetInBuf - 1));
        DBG_Printf("*(IDb)=0x%x\n", *(U8 *)(ulByteOffsetInBuf));
        //DBG_Printf("*(IDb+1)=0x%x\n", *(U8 *)(ulByteOffsetInBuf + 1));
    }
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

 History        :
 1.Date         : 2014/4/2
   Author       : henryluo
   Modification : Created function
 2.Date         : 2014/11/28
   Author       : JasonGuo
   Modification : Modify, L95-16nmChip, need to meet the share pages rules.
-----------------------------------------------------------------------------*/
U32 HAL_IsFlashPageSafed(U8 ucPu, U16 usWritePPO, U16 usReadPPO)
{
    U16 usSafeDelta;

    if (usWritePPO < usReadPPO-1) // allowed to read emptypage when move the open block
    {
        DBG_Printf("Flash Page isn't safe. Pu = %d, usWritePPO = %d, usReadPPO = %d, getch\n", ucPu, usWritePPO, usReadPPO);
        DBG_Getch();
    }

    usSafeDelta = usWritePPO - usReadPPO;
    if (usSafeDelta < PAIR_PAGE_INTERVAL_MAX)
    {
        //DBG_Printf("MCU#%d Flash Page isn't safe. usWritePPO = %d, usReadPPO = %d\n", HAL_GetMcuId(), usWritePPO, usReadPPO);
        return FALSE;
    }

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
    call when program TSB 2D TLC block in special order, other flash just return usPrgIndex for compile
History:
    20160206     abby      add header
------------------------------------------------------------------------------*/
U16 HAL_FlashGetTlcPrgWL(U16 usPrgIndex)
{
    return usPrgIndex;
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
    call when program TSB 2D TLC block in special order, other flash just return 0 for compile
History:
    20160206    abby    add header
    20160226    abby    support 3D TLC, prg cycle always equal 0
------------------------------------------------------------------------------*/
U8 HAL_FlashGetTlcPrgCycle(U16 usPrgIndex)
{
    return 0;
}

U8 HAL_FlashGetScrSeed(U16 usPage, U8 ucScrMod, U8 ucPageType)
{
    return usPage & MSK_7F;
}


/*   end of this file   */
