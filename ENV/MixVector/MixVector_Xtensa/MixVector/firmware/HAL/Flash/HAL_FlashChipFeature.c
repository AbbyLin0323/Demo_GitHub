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
Description : 
Others      : 
Modify      :
*******************************************************************************/

#include "HAL_FlashCmd.h"
#include "HAL_HostInterface.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_NormalDSG.h"
#include "COM_Memory.h"
#ifdef SIM
#include "sim_flash_shedule.h"
#endif

LOCAL U16 l_aSLCPageMap[PG_PER_BLK/2];
LOCAL MCU12_VAR_ATTR U8 l_aReadRetryVal[HAL_FLASH_MAX_RETRY_TIME];

/*------------------------------------------------------------------------------
Name: HAL_FlashNfcFeatureInit
Description: 
    Initialize additional features which are needed by Normal FW 
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
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashNfcFeatureInit(void)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg; 

    /* ECC mode/threshold */    
    pNfcPgCfg->EccTh  = 0x18;
    
    /* PRCQ depth */
    pNfcPgCfg->PrcqDepth = PRCQDEPTH_16DW; 

    /* Page/Block/Red size */
    pNfcPgCfg->PgSz = PGSIZE_16K; 
    
#ifndef VT3514_C0
    pNfcPgCfg->EccSel = ECCSEL_40;
    pNfcPgCfg->RedNum = REDNUM_64BYTE;
    pNfcPgCfg->BlkSz = BLKSIZE_512;
#else
    pNfcPgCfg->EccSel = ECCSEL_40;
    pNfcPgCfg->RedNum = REDNUM_48BYTE;
    pNfcPgCfg->bsCloseHWCmdPatch = 0;
    pNfcPgCfg->ScrRotEn = 0;
    pNfcPgCfg->MulLun = LUN_NUM_BITS;
#endif

    /*scramble disable*/
    rNfcTCtrl1 |= (1<<4);

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

    for(ulLogicPage = 0; ulLogicPage < PG_PER_BLK/2; ulLogicPage++)
    {
        if(ulLogicPage < 6)
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage;
            continue;
        }

        if(EVEN == (ulLogicPage%2))
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage*2 - 4;
        }
        else if(ODD == (ulLogicPage%2))
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage*2 - 5;
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashInitSLCMapping
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
U16 MCU12_DRAM_TEXT HAL_FlashGetSLCPage(U16 usLogicPage)
{
    return l_aSLCPageMap[usLogicPage];
}


/*------------------------------------------------------------------------------
Name: HAL_FlashReadRetryCheck
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
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_FlashReadRetryCheck(U8 ucTime)
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
Name: HAL_FlashSendRetryPreConditon
Description: 
    send retry pre-condition command to flash
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    BOOL: TRUE = send command success, FALSE = fail
Usage:
    called before do read retry sequence
History:
    20140911    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashSendRetryPreConditon(U8 ucPU)
{
    return NFC_STATUS_SUCCESS;//L85 flash do not need any actual precondion
}

/*------------------------------------------------------------------------------
Name: HAL_FlashSendReadRetryCmd
Description: 
    send retry command to flash
Input Param:
    U8 ucPU: PU number
    U8 ucTime: which time for retry
Output Param:
    none
Return Value:
    BOOL: TRUE = send command sucess, FALSE = fail
Usage:
    called when do read retry sequence
History:
    20140911    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashSendReadRetryCmd(U8 ucPU, U8 ucTime)
{
    U32 ulData;
    ulData = HAL_FlashGetReadRetryVal(ucTime);
#ifdef SIM
    g_RetryTimes[HAL_NfcGetCE(ucPU)] = ucTime+1;
#endif

    return HAL_NfcSetFeature(ucPU, ulData, 0x89);
}

/*------------------------------------------------------------------------------
Name: HAL_FlashReadRetryTerminate
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
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_FlashReadRetryTerminate(U8 ucPU)
{
#ifdef SIM
    g_RetryTimes[HAL_NfcGetCE(ucPU)] = 0;
#endif

    while (NFC_STATUS_SUCCESS != HAL_NfcSetFeature(ucPU, 0, 0x89));  

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashRetryValueInit
Description: 
    init read retry configuration value.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in FW init sequence
History:
    20140911    Gavin   modify to meet coding style
    20141024    Gavin   change this function to "LOCAL"
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashRetryValueInit(void)
{
    l_aReadRetryVal[0] = 0;
    l_aReadRetryVal[1] = 1;
    l_aReadRetryVal[2] = 2;
    l_aReadRetryVal[3] = 4;
    l_aReadRetryVal[4] = 5;
    l_aReadRetryVal[5] = 6;
    l_aReadRetryVal[6] = 7;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashGetReadRetryVal
Description: 
    get read retry value
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in read retry sequence
History:
    20150121 tobey move form HAL_FlashDriverExt.c
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_FlashGetReadRetryVal(U8 ucTime)
{
    return l_aReadRetryVal[ucTime];
}

/*------------------------------------------------------------------------------
Name: HalNfcGetPlnNumFromNfcq
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
------------------------------------------------------------------------------*/
LOCAL U8 MCU12_DRAM_TEXT HalNfcGetPlnNumFromNfcq(NFCQ_ENTRY * pNfcqEntry)
{
    U8 ucPln;
    ucPln = (pNfcqEntry->aRowAddr[0]>>PG_PER_BLK_BITS) & PLN_PER_PU_BITS;

    return ucPln;
}

/*------------------------------------------------------------------------------
Name: HAL_FlashReadRetry
Description: 
    flash read retry interface.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Need handle FullPageRead, SinglePageRead, PageReadByteMode.
History:
    20150121 tobey move form HAL_FlashDriverExt.c
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_FlashReadRetry(U8 Pu, U8 ReqType, U8 SecStart, U8 SecLen, U16 BuffID, NFC_RED **ppOrgRedEntry)
{
    U8 ucPln;
    U8  ucRetryTime,ucErrCode;
    U16 usCurDsgId;
    U32 ulCmdStatus;
    NFCQ_ENTRY *pCqEntry,CqBak;
    FLASH_ADDR PhyAddr = {0};   
    NORMAL_DSG_ENTRY *pDSG;
        
    pCqEntry = HAL_NfcGetNfcqEntryAddr(Pu,TRUE);
    COM_MemCpy((U32 *)&CqBak, (U32 *)pCqEntry, sizeof(NFCQ_ENTRY)/sizeof(U32));

    // reset Pu
    HAL_NfcResetCmdQue(Pu);
    HAL_NfcClearINTSts(Pu);
    
    // Step1. pre-condition: enter to shift read mode.
    ulCmdStatus = HAL_FlashSendRetryPreConditon(Pu);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {
        DBG_Printf("MCU#%d:ReadRetry-PreConditon PU#%d: You shouidn't be in here!\n", HAL_GetMcuId(), Pu);
        DBG_Getch();
    }
    ulCmdStatus = HAL_NfcWaitStatus(Pu);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {
        DBG_Printf("MCU#%d:ReadRetry-PreConditon-WaitStatus PU#%d: You shouidn't be in here!\n", HAL_GetMcuId(), Pu);
        DBG_Getch();
    }
    
    ucRetryTime = 0;
    while (NFC_STATUS_SUCCESS == HAL_FlashReadRetryCheck(ucRetryTime))
    {
        // Step2. set-parameters: adj voltage.        
        ulCmdStatus = HAL_FlashSendReadRetryCmd(Pu, ucRetryTime);
        if (NFC_STATUS_SUCCESS != ulCmdStatus)
        {
            DBG_Printf("MCU#%d:ReadRetry-CmdSend PU#%d: You shouidn't be in here!\n", HAL_GetMcuId(), Pu);
            DBG_Getch();
        }
        ulCmdStatus = HAL_NfcWaitStatus(Pu);
        if (NFC_STATUS_SUCCESS != ulCmdStatus)
        {
            DBG_Printf("MCU%d:ReadRetry-CmdSend-WaitStatus PU#%d: You shouidn't be in here!\n", HAL_GetMcuId(), Pu);
            DBG_Getch();
        }
        
        // Step3. read-retry:
        pCqEntry = HAL_NfcGetNfcqEntryAddr(Pu,FALSE);
        COM_MemCpy((U32 *)pCqEntry, (U32 *)&CqBak, sizeof(NFCQ_ENTRY)/sizeof(U32));

        // update the red
        if (NULL != ppOrgRedEntry)
        {
            pCqEntry->bsRedEn   = TRUE;
            ucPln = HalNfcGetPlnNumFromNfcq(pCqEntry);
            pCqEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(Pu, HAL_NfcGetWP(Pu), ucPln);
            *ppOrgRedEntry = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(Pu, HAL_NfcGetWP(Pu), ucPln);
        }

        if (0 != SecLen)
        {
            // update the dsg           
            while(FALSE == HAL_GetNormalDsg(&usCurDsgId));
            pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
            COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);    
            if (NF_PRCQ_READ_1PLN == ReqType)
            {
                pDSG->ulDramAddr = HAL_NfcGetDmaAddr(BuffID, SecStart, PIPE_PG_SZ_BITS-PLN_PER_PU_BITS);
            }
            else
            {
                pDSG->ulDramAddr = HAL_NfcGetDmaAddr(BuffID, SecStart, PIPE_PG_SZ_BITS);
            }
            pDSG->bsXferByteLen = SecLen << SEC_SIZE_BITS ;
            pDSG->bsLast        = TRUE;
            HAL_SetNormalDsgSts(usCurDsgId,1);
            
            pCqEntry->bsFstDsgPtr = usCurDsgId;
        }

        HAL_NfcSetPRCQEntry(Pu,ReqType); 
        PhyAddr.ucPU = Pu;
        HAL_NfcCmdTrigger(&PhyAddr,ReqType);

        ulCmdStatus = HAL_NfcWaitStatus(Pu);
        if (NFC_STATUS_SUCCESS != ulCmdStatus)
        {
            ucErrCode = HAL_NfcGetErrCode(Pu);
            if ((NF_ERR_TYPE_UECC     != ucErrCode)
             && (NF_ERR_TYPE_CRC      != ucErrCode)
             && (NF_ERR_TYPE_TWO_UECC != ucErrCode))
            {
                DBG_Printf("MCU#%d: PU#%d ReadRetry-ErrCode Changed To %d.\n", HAL_GetMcuId(),Pu,ucErrCode);
                ucErrCode = NF_ERR_TYPE_UECC;                
            }
            
            HAL_NfcResetCmdQue(Pu);
            HAL_NfcClearINTSts(Pu);

            ucRetryTime++;
            DBG_Printf("MCU#%d: PU#%d Blk#%d Pg#%d ReadRetry ErrType=%d  CurTime:%d.\n",
            HAL_GetMcuId(), Pu, 
            (pCqEntry->aRowAddr[0]>>PG_PER_BLK_BITS>>PLN_PER_PU_BITS)&0x3FF,
            pCqEntry->aRowAddr[0]&0x1FF,ucErrCode,
            ucRetryTime);
        }
        else
        {     
            ucErrCode = NF_SUCCESS;
            break;
        }
    }

    // Step4. terminate retry: enter to normal mode
    ulCmdStatus = HAL_FlashReadRetryTerminate(Pu);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {
        DBG_Printf("MCU#%d:ReadRetry-Terminate PU#%d: You shouidn't be in here!\n", HAL_GetMcuId(), Pu);
        DBG_Getch();
    }   
    ulCmdStatus = HAL_NfcWaitStatus(Pu);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {
        DBG_Printf("MCU#%d:ReadRetry-Terminate-WaitStatus PU#%d: You shouidn't be in here!\n", HAL_GetMcuId(), Pu);
        DBG_Getch();
    }

    if(NFC_STATUS_FAIL == HAL_FlashReadRetryCheck(ucRetryTime))
    {
        DBG_Printf("MCU#%d: PU#%d ReadRetry Fail!\n", HAL_GetMcuId(), Pu);
    }
    else
    {    
        DBG_Printf("MCU#%d: PU#%d ReadRetry Success!\n", HAL_GetMcuId(), Pu);
    }

    return ucErrCode;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcWaitStatusWithRetry
Description: 
    wait flash status, do read retry is error occur.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
History:
    20150121 tobey move form HAL_FlashDriverExt.c
    20150311 jasonguo delete HAL_NFC_RD_RETRY macro
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_NfcWaitStatusWithRetry(U8 Pu, U8 ReqType, U8 SecStart, U8 SecLen, U16 BuffID, NFC_RED **ppOrgRedEntry)
{
    U8  ucErrCode = NF_SUCCESS;
    U32 ulCmdStatus;
       
    ulCmdStatus = HAL_NfcWaitStatus(Pu);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {                
        ucErrCode = HAL_NfcGetErrCode(Pu);
        if ((NF_ERR_TYPE_UECC    == ucErrCode)
         || (NF_ERR_TYPE_CRC     == ucErrCode)
         || (NF_ERR_TYPE_TWO_UECC== ucErrCode))
        {
            ucErrCode = HAL_FlashReadRetry(Pu,ReqType,SecStart,SecLen,BuffID,ppOrgRedEntry);
        }    
    }
    
    return ucErrCode;
}


/*
NAND Flash devices are shipped from the factory erased. The factory identifies invalid
blocks before shipping by attempting to program the bad-block mark into every loca-tion 
in the first page of each invalid block. It may not be possible to program every loca-tion
with the bad-block mark. However, the first spare area location in each bad block is
guaranteed to contain the bad-block mark. This method is compliant with ONFI Facto-ry 
Defect Mapping requirements. See the following table for the first spare area location
and the bad-block mark.
*/
BOOL MCU12_DRAM_TEXT HAL_FlashIsBlockBad(U8 ucPu,U8 ucPln, U16 usBlk)
{
    U8 ucErrCode;
    FLASH_ADDR PhyAddr;
    NFC_RED *pRedAddr;

    COM_MemZero((U32 *)&PhyAddr, sizeof(FLASH_ADDR)>>2);
    PhyAddr.ucPU = ucPu;
    PhyAddr.bsPln = ucPln;
    PhyAddr.usBlock = usBlk;
    PhyAddr.usPage = 0;

    HAL_NfcByteModeRead(&PhyAddr,BAD_BLK_MARK_POS,1,&pRedAddr);

    ucErrCode = HAL_NfcWaitStatusWithRetry(ucPu, NF_PRCQ_READ_1PLN,0,0,0,&pRedAddr);
    if(NF_SUCCESS != ucErrCode)
    {
        HAL_NfcResetCmdQue(ucPu);
        HAL_NfcClearINTSts(ucPu);
        return TRUE;
    }
    else
    {
        //check read byte 0x0 means bad blk
        if((pRedAddr->aContent[0]&0xff) == 0x0)
        {
            return TRUE;
        }
    }

    return FALSE;
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
-----------------------------------------------------------------------------*/
U32 HAL_IsFlashPageSafed( U16 usWritePPO, U16 usReadPPO)
{
    if (usWritePPO < usReadPPO)
    {
        DBG_Printf("MCU#%d Flash Page isn't safe. usWritePPO = %d, usReadPPO = %d, getch\n", HAL_GetMcuId(), usWritePPO, usReadPPO);
        DBG_Getch();
    }

    if (usWritePPO == usReadPPO)
    {    
        //DBG_Printf("MCU#%d Flash Page isn't safe. usWritePPO = %d, usReadPPO = %d\n", HAL_GetMcuId(), usWritePPO, usReadPPO);
        return FALSE;
    }

    return TRUE;
}
