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
Filename    : HAL_FlashDriverBasic.c
Version     : Ver 1.0
Author      : Tobey
Date        : 2014.09.05
Description : 
Others      : 
Modify      :
20140905    Tobey     Create file
*******************************************************************************/

#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashCmd.h"
#include "HAL_NormalDSG.h"
#include "HAL_ParamTable.h"
#include "HAL_HostInterface.h"
#include "HAL_FlashDriverBasic.h"
#ifdef SIM
#include "sim_flash_common.h"
#include "win_bootloader.h"
#endif

GLOBAL MCU12_VAR_ATTR volatile NFC_PRCQ *g_pNfcPRCQ;
GLOBAL MCU12_VAR_ATTR volatile NFC_PRCQ_QE *g_pNfcPRCQQe;
GLOBAL MCU12_VAR_ATTR volatile NFCQ *g_pNfcq;
GLOBAL MCU12_VAR_ATTR volatile NFC_CMD_STS *g_pNfcCmdSts;
GLOBAL MCU12_VAR_ATTR volatile NFC_TRIGGER *g_pNfcTrigger;
GLOBAL MCU12_VAR_ATTR volatile NFC_SIM_CMDTYPE *g_pNfcSimCmdType;
GLOBAL MCU12_VAR_ATTR volatile NFC_LOGIC_PU  *g_pNfcLogicPU;
GLOBAL MCU12_VAR_ATTR U32 g_ulRedDataBase;

LOCAL MCU12_VAR_ATTR U8 l_aLogicPUMap[CE_MAX]; // PU to logic PU map
LOCAL MCU12_VAR_ATTR U8 l_aCEMap[CE_MAX];      // PU to CE map

LOCAL MCU12_VAR_ATTR U8 l_aPU2LUN[CE_MAX];
LOCAL MCU12_VAR_ATTR PU_BROTHER l_aPUBrother[CE_MAX];

extern MCU12_VAR_ATTR PRCQ_TABLE g_aPrcqTable[];
extern GLOBAL MCU12_VAR_ATTR U32 g_ulPuNum; 

/*------------------------------------------------------------------------------
Name: HalNfcIsTrigInsert
Description: 
    Insert bit indicates the next available read/write request could be issued as
    a flash array (or cache) command operation.
    Here we assume that a serial requests have the same command type (read or write type)
    that could be processed as a cache operation.
    For the given PU ,after the former read request be triggered,while the current
    read request was triggered,with the insert bit be set, in time ,
    then the NFC would reconstruct the prcq sequences of the read requests as one cache operation
    prcq sequence.

    This bit also be used in hardware oriented multiple LUN operation.

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
------------------------------------------------------------------------------*/
LOCAL BOOL HalNfcIsTrigInsert(U8 ucCurrCmdType)
{
    BOOL ret = FALSE;

#ifdef FLASH_CACHE_OPERATION
    if ((NF_PRCQ_READ_2PLN == ucCurrCmdType)||(NF_PRCQ_PRG_2PLN == ucCurrCmdType)) 
    {
        ret = TRUE;
    }
#endif 

    return ret;
}

/*------------------------------------------------------------------------------
Name: HalNfcIsTrigCacheOp
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
------------------------------------------------------------------------------*/
LOCAL BOOL HalNfcIsTrigCacheOp(U8 ucCurrCmdType)
{
    BOOL ret = FALSE;

#ifdef FLASH_CACHE_OPERATION
    if ((NF_PRCQ_READ_2PLN == ucCurrCmdType)||(NF_PRCQ_PRG_2PLN == ucCurrCmdType)) 
    {
        ret = TRUE;
    }
#endif 

    return ret;
}


/*------------------------------------------------------------------------------
Name: HalNfcGetCmdTypeAddr
Description: 
    Get a PU's current Wp directing cmd type address. 
    caution, cmd type entry is ordered by CE.
Input Param:
    U8 ucCE: CE number
    U8 ucWp: write pointer
Output Param:
    none
Return Value:
    U32: cmd type address.
Usage:
    FW call this function if it wants obtain a PU's current Wp directing cmd type address. 
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
LOCAL U32 HalNfcGetCmdTypeAddr(U8 ucCE, U8 ucWp)
{
    return (U32)(&g_pNfcSimCmdType->aCmdType[ucCE][ucWp]);
}

/*------------------------------------------------------------------------------
Name: HalNfcSetCmdType
Description: 
    set corresponding cmd code before trigger, this is only need on XTMP and SIM ENV.
Input Param:
    U8 ucCE: CE.
    U8 ucWp: Wp
    U8 ucReqType: cmd request type
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set corresponding cmd code.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
LOCAL void HalNfcSetCmdType(U8 ucCE, U8 ucWp, U8 ucReqType)
{
    PRCQ_TABLE *pPrcqTable;   
    U8 *pCmdTyepAddr;

#if (defined(XTMP) || defined(SIM))   
    pCmdTyepAddr = (U8 *)HalNfcGetCmdTypeAddr(ucCE, ucWp);
    pPrcqTable = HAL_NfcGetPrcqEntry(ucReqType);
    *pCmdTyepAddr = pPrcqTable->bsCmdCode;
#endif    

    return;
}


#ifdef COSIM
/*------------------------------------------------------------------------------
Name: HalNfcGetCEBitMap
Description: 
    Get CE BitMap in Cosim env.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In FW. call this funtion to get ce bitmap in cosim env.
History:
    20141027    Jason   create.
------------------------------------------------------------------------------*/
LOCAL U32 MCU12_DRAM_TEXT HalNfcGetCEBitMap(U32 ulMcuID)
{
    U32 ulBitMap;
    
    if (MCU1_ID == ulMcuID)
    {
        ulBitMap = DEF_SUB1_BITMAP;
    }
    else if (MCU2_ID == ulMcuID)
    {
         ulBitMap = DEF_SUB2_BITMAP;
    }
    else
    {
        DBG_Printf("McuID=%d Err.\n", ulMcuID);
        DBG_Getch();
    }

    return ulBitMap;
}
#endif

/*------------------------------------------------------------------------------
Name: HAL_NfcGetCEBitMap
Description: 
    Get the CE BitMap of the current subsystem
Input Param:
    U32 ulMcuID: current subsystem id.
Output Param:
    none
Return Value:
    U32 ulSubSystemBitMap: current subsystem ce bitmap.
Usage:
    In FW. call this funtion to get the CE BitMap of the current subsystem
History:
    20140911    Tobey   create.
    20141027    Jason   modify.
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcGetCEBitMap(U32 ulMcuID)
{
    U32 ulSubSystemBitMap;

#if defined(COSIM)
    ulSubSystemBitMap = HalNfcGetCEBitMap(ulMcuID); // custom redefined
#else
    ulSubSystemBitMap = HAL_GetSubSystemCEMap(ulMcuID);// from param-table
#endif

    return ulSubSystemBitMap;
}

/*------------------------------------------------------------------------------
Name: HalNfcPU2CEMapping
Description: 
    Initialize PU to CE map for BootLoader according CE BitMap.
Input Param:
    U32 ulBitMap: CE Bit Map
Output Param:
    none
Return Value:
    none
Usage:
    In BootLoader. call this funtion to Initialize PU to CE map for BootLoader.
History:
    20140911    Tobey   create.
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HalNfcPU2CEMapping(U32 ulBitMap)
{
    U8 ucCE;
    U8 ucPU = 0;

    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        if (0 != (ulBitMap & (1 << ucCE)))
        {
            l_aCEMap[ucPU] = ucCE;
            ucPU++;
        }
    }

    for (; ucPU < CE_MAX; ucPU++)
    {
        l_aCEMap[ucPU] = INVALID_2F;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcPU2LgcPUMapping
Description: 
    Initialize FW PU to LogicPU mapping
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In FW. call this funtion to Initialize FW PU to LogicPU mapping
History:
    20140911    Tobey   create.
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HalNfcPU2LgcPUMapping(void)
{
    U8 Pu;
    U32 ulMcuID;
    
    ulMcuID = HAL_GetMcuId();
    if (MCU0_ID == ulMcuID)
    {
        for  (Pu = 0; Pu < CE_MAX; Pu++)
        {
            l_aLogicPUMap[Pu] = Pu;
        }
    }
    else
    {
        for  (Pu = 0; Pu < SUBSYSTEM_PU_NUM; Pu++)
        {
            l_aLogicPUMap[Pu] = Pu + (ulMcuID - MCU1_ID)*SUBSYSTEM_PU_NUM;
        }

        for(; Pu < CE_MAX; Pu++)
        {
            l_aLogicPUMap[Pu] = INVALID_2F;
        }
    }

    return;
}

U8 HalCalcChannalNum(U32 ulCEMap)
{ 
    U8 ucChanNum;
    U8 ucPuInChan;
    U8 ucCENum;
    U8 ucChanlCnt = 0;

    for (ucChanNum = 0; ucChanNum < NFC_CH_TOTAL; ucChanNum++)
    {
        for (ucPuInChan = 0; ucPuInChan < NFC_PU_PER_CH; ucPuInChan++)
        {
            ucCENum = ucPuInChan * NFC_CH_TOTAL + ucChanNum;
            if (0 != (ulCEMap & (1<<ucCENum)))
            {
                ucChanlCnt++;
                break;
            }
        }
    }

    return ucChanlCnt;
}

LOCAL void HalInitPUBrother(U32 ulBitMap)
{
    U8 ucPU;
    U8 ucLun;
    U8 ucBrotherPU;
    U8 ucChanNum;
    U8 ucValidPuNum;
    U8 ucBrotherIndex;

    ucChanNum = HalCalcChannalNum(ulBitMap);
    ucValidPuNum = (MCU0_ID == HAL_GetMcuId()) ? CE_MAX : SUBSYSTEM_PU_NUM;

    for (ucPU = 0; ucPU < ucValidPuNum; ucPU++)
    {
        ucBrotherIndex = 0;
        for (ucLun = 0; ucLun < LUN_NUM; ucLun++)
        {
            ucBrotherPU = ucPU%ucChanNum + ucLun*ucChanNum+ (LUN_NUM*ucChanNum)*(ucPU/(LUN_NUM*ucChanNum));
            if(ucPU != ucBrotherPU)
            {
                l_aPUBrother[ucPU].aBrother[ucBrotherIndex] = ucBrotherPU;
                DBG_Printf("ucPU %d, brothter %d,\n", ucPU, ucBrotherPU);
                ucBrotherIndex++;
            }
        }
    }

    return;
}

GLOBAL U8 HAL_NfcGetBrPu(U8 ucPu, U8 ucIndex)
{
    return l_aPUBrother[ucPu].aBrother[ucIndex];
}

/*------------------------------------------------------------------------------
Name: HAL_NfcPuMapping
Description: 
    Initialize FW PU to CE mapping  and PU to LogicPU mapping
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In FW. call this funtion to Initialize FW PU to CE mapping  and PU to LogicPU mapping
History:
    20140911    Tobey   create.
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcPuMapping(U32 ulBitMap)
{
    HalNfcPU2CEMapping(ulBitMap);
    
    HalNfcPU2LgcPUMapping();

    HAL_NfcInitPU2LUN();
    HalInitPUBrother(ulBitMap);

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetLogicPUReg
Description: 
    Initialize LogicPUReg according to two subsystem CE to LogicPU bit map.
Input Param:
    U32 ulSubsys0CEBitMap: CE to LogicPU map for FW processer 0.
    U32 ulSubsys1CEBitMap: CE to LogicPU map for FW processer 1.
Output Param:
    none
Return Value:
    none
Usage:
    In BootLoader. call this funtion to Initialize LogicPU register for FW.
History:
    20140911    Tobey   create.
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcSetLogicPUReg(U32 ulSubsys0CEBitMap, U32 ulSubsys1CEBitMap)
{   
    U8 ucCE;   
    U8 ucLogicPU = 0;

    if (0 != (ulSubsys0CEBitMap & ulSubsys1CEBitMap))
    {
        DBG_Getch();
    }
    
    /* set LogicPU register into a no map sts */
    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable = FALSE;   
        g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = 0;                                                              
    }

    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        if (0 != (ulSubsys0CEBitMap & (1 << ucCE)))
        {
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable = TRUE;   
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = ucLogicPU;       
            ucLogicPU++;
        }       
    }

    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        if (0 != (ulSubsys1CEBitMap & (1 << ucCE)))
        {
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable = TRUE;   
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = ucLogicPU;       
            ucLogicPU++;
        }       
    }

    // init the left hw reg
    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        if ((0 == g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable)
         && (0 == g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU))
        {
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = ucLogicPU++;
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcSetRedDataBase
Description: 
    Initialize global variable g_ulRedDataBase.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In BootLoader and FW. call this funtion to g_ulRedDataBase.
History:
    20140911    Tobey   create.
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcSetRedDataBase(void)
{
    U32 ulMcuId;

    ulMcuId = HAL_GetMcuId();
    if (MCU2_ID == ulMcuId)
    {
        g_ulRedDataBase = OTFB_RED_DATA_MCU2_BASE;
    }
    else
    {
        g_ulRedDataBase = OTFB_RED_DATA_MCU1_BASE;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcInterfaceInitBasic
Description: 
    Basic NFC relative base address Initialization.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    This funtion is enought for bootloader and rom nfc base address Initialization.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20141024    Gavin   move Red pointer initialization to last step
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcInterfaceInitBasic(void)
{
    /* NFCQ & DPTR */
    g_pNfcq = (NFCQ*)CQ_ENTRY_BASE;        
    g_pNfcCmdSts = (NFC_CMD_STS*)NFC_CMD_STS_BASE;
    
#if (defined(XTMP) || defined(SIM))
    g_pNfcSimCmdType = (NFC_SIM_CMDTYPE*)(NFC_CMD_STS_BASE + sizeof(NFC_CMD_STS));
#endif

    /* PU Mapping */
    g_pNfcLogicPU = (NFC_LOGIC_PU *)LOGIC_PU_REG_BASE;

    /* PRCQ entry */    
    g_pNfcPRCQ = (NFC_PRCQ*)PRCQ_ENTRY_BASE;
    g_pNfcPRCQQe = (NFC_PRCQ_QE*)NF_QE_REG_BASE;

    /* Trigger */
    g_pNfcTrigger = (NFC_TRIGGER*)TRIGGER_REG_BASE;

    /* PRCQ relative init */
    HAL_NfcQEEInit();
    HAL_NfcPrcqTableInit();

    /* init FW pointer to redundant area */    
    HAL_NfcSetRedDataBase();

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetCE
Description: 
    Get a PU's corresponding CE number.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: CE number.
Usage:
    FW call this function if it wants obtain a PU's corresponding CE number.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
U8 HAL_NfcGetCE(U8 ucPU)
{
    return l_aCEMap[ucPU];
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetLogicPU
Description: 
    Get a PU's corresponding Logic PU number.
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    U8: LogicPU number
Usage:
    FW call this function if it wants obtain a PU's corresponding Logic PU number.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
U8 HAL_NfcGetLogicPU(U8 ucPU)
{
    return l_aLogicPUMap[ucPU];
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetWP
Description: 
    Get a PU's write pointer.
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    U8: PU's write pointer
Usage:
    FW call this function if it wants obtain a PU's write pointer.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
U8 HAL_NfcGetWP(U8 ucPU)
{
    U8 ucCE;
    ucCE = HAL_NfcGetCE(ucPU);
    return (U8)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsWrPtr;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetWP
Description: 
    Get a PU's read pointer.
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    U8: PU's read pointer
Usage:
    FW call this function if it wants obtain a PU's read pointer.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
U8 HAL_NfcGetRP(U8 ucPU)
{
    U8 ucCE;
    ucCE = HAL_NfcGetCE(ucPU);
    return (U8)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsRdPtr;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetIdle
Description: 
    Get a PU's idle sts. HW set idle when all cmd done or error hold.
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    BOOL: 1: idle
          0: not idle
Usage:
    FW call this function if it wants obtain a PU's idle sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetIdle(U8 ucPU)
{
    U8 ucCE;

    ucCE = HAL_NfcGetCE(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsIdle;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetFull
Description: 
    Get a PU's full sts. HW set full when all HW cmd queue filled.
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    BOOL: 1: full
          0: not full
Usage:
    FW call this function if it wants obtain a PU's full sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetFull(U8 ucPU)
{
    U8 ucCE;
    ucCE = HAL_NfcGetCE(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsFull;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetEmpty
Description: 
    Get a PU's empty sts. HW set empty when all HW cmd queue not filled.
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    BOOL: 1: empty
          0: not empty
Usage:
    FW call this function if it wants obtain a PU's empty sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetEmpty(U8 ucPU)
{
    U8 ucCE;
    ucCE = HAL_NfcGetCE(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsEmpty;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetErrHold
Description: 
    Get a PU's error sts. HW set bsErrh when error occur.
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    BOOL: 1: error hold
          0: no error
Usage:
    FW call this function if it wants obtain a PU's error sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetErrHold(U8 ucPU)
{
    U8 ucCE;
    ucCE = HAL_NfcGetCE(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsErrh;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetErrCode
Description:
    get PU error status.
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    U8: error status.
Usage:
    anywhere need check NFC error status.
History:
    20140912    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
U8 HAL_NfcGetErrCode(U8 ucPU)
{
    U8 ucCE;
    
    ucCE = HAL_NfcGetCE(ucPU);
    
    return (U8)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsErrSts;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetFinish
Description:
    get PU one level's finish sts.
Input Param:
    U8 ucPU: PU number
    U8 ucLevel: HW queue num
Output Param:
    none
Return Value:
    BOOL:TURE: finished; FALSE:not finished;
Usage:
    anywhere need check NFC error status.
History:
    20140912    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetFinish(U8 ucPU, U8 ucLevel)
{
    U8 ucCE;
    BOOL bFinishFlag = FALSE;

    ucCE = HAL_NfcGetCE(ucPU);

    if (0 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsFsLv0;
    }
    else if (1 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsFsLv1;
    }
#ifdef VT3514_C0
    else if (2 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsFsLv2;
    }
    else if(3 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsFsLv3;
    }
#endif
    else
    {
        DBG_Getch();
    }

    return bFinishFlag;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetDmaAddr
Description: 
    Get a offset addr accordint to BuffId and SecInBuff. at present 64 sec one buff.
Input Param:
    U32 ulBuffID: buff id
    U32 ulSecInBuff: sec number in buff
Output Param:
    none
Return Value:
    U32: addr
Usage:
    FW call this function if it wants obtain a offset addr accordint to BuffId and SecInBuff.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
U32 HAL_NfcGetDmaAddr(U32 ulBuffID, U32 ulSecInBuff, U32 ulBufSizeBits)
{
    return ((ulBuffID << ulBufSizeBits) + (ulSecInBuff << SEC_SIZE_BITS));
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetNfcqEntryAddr
Description: 
    Get a PU's current Wp directing NFCQ entry address. 
    caution, NFCQ entry is ordered by LogicPU.
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    U32: NFCQ entry address.
Usage:
    FW call this function if it wants obtain a PU's current Wp directing NFCQ entry address.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
NFCQ_ENTRY * HAL_NfcGetNfcqEntryAddr(U8 ucPU,BOOL bRp)
{
    U8 ucLevel;
    if (FALSE == bRp)
    {
        ucLevel = HAL_NfcGetWP(ucPU);   
    }
    else
    {
        ucLevel = HAL_NfcGetRP(ucPU);
    }

    return (NFCQ_ENTRY *)(&g_pNfcq->aNfcqEntry[HAL_NfcGetLogicPU(ucPU)][ucLevel]);
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetPRCQEntryAddr
Description: 
    Get a PU's current Wp directing PRCQ entry address. 
    caution, PRCQ entry is ordered by LogicPU.
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    U32: PRCQ  entry address.
Usage:
    FW call this function if it wants obtain a PU's current Wp directing PRCQ entry address.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
U32 HAL_NfcGetPRCQEntryAddr(U8 ucPU)
{
    U8 ucWp;    
    ucWp = HAL_NfcGetWP(ucPU);   
    return (U32)(&g_pNfcPRCQ->aNfcPRCQEntry[HAL_NfcGetLogicPU(ucPU)][ucWp]);
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetFlashRowAddr
Description: 
    convert flash addr descriptor to flash Row address.  
    HW need this type of address.
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to flash addr descriptor.
Output Param:
    none
Return Value:
    U32: flash array address.
Usage:
    FW call this function if it wants convert flash addr descriptor to flash array address.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
U32 HAL_NfcGetFlashRowAddr(FLASH_ADDR * pFlashAddr)
{
    U32 addr;
    U8 ucLun;

    ucLun = HAL_NfcGetLUN(pFlashAddr->ucPU);

    if (pFlashAddr->usPage < PG_PER_BLK)// for TSB 19ns, page in block is 256, while L85 is 512
    {
        addr = ( ucLun<< BIT_LUN_IN_ROW_ADDR)
             | (((pFlashAddr->usBlock << PLN_PER_PU_BITS) | pFlashAddr->bsPln) << PG_PER_BLK_BITS) 
             | pFlashAddr->usPage ;
    }

    return addr;    
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetPRCQEntry
Description: 
    set a PU's nfcq entry according to ReqType, ReqType is equal to the flash cmd index.
Input Param:
    U8 ucPU: PU number.
    U8 ucReqType: flash request type.
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set a PU's nfcq entry according to ReqType.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
void HAL_NfcSetPRCQEntry(U8 ucPU, U8 ucReqType)
{
    U32 ulPrcqAddr;
    U32 ulBufAddr;
    U32 ulPhaseDepth;
    U32 ulPhase;

    ulPrcqAddr = HAL_NfcGetPRCQEntryAddr(ucPU);
    ulBufAddr = g_aPrcqTable[ucReqType].bsQEPtr;
    ulPhaseDepth =  g_aPrcqTable[ucReqType].bsQEPhase;
    
    for (ulPhase = 0; ulPhase < ulPhaseDepth; ulPhase++)
    {
        if (TRUE == g_aPrcqTable[ucReqType].bsIsPIO)
        {
            *((U32*)ulPrcqAddr + ulPhase) = *((U32*)ulBufAddr + ulPhase);
        }
        else
        {
            *((U8*)ulPrcqAddr + ulPhase) = *((U8*)ulBufAddr + ulPhase);
        }
    }
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcResetCmdQue
Description: 
    reset a PU's HW cmd queue into a avialble sts. 
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to reset a PU's HW cmd queue into a avialble sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20141127    Tobey   modify note.
    20150119    Tobey   HW don't support set function
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcResetCmdQue(U8 ucPU)
{
    U8 ucCE;
    
    ucCE = HAL_NfcGetCE(ucPU);

    g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsPrst = TRUE;

#ifdef SIM
    NFC_InterfaceResetCQ(ucCE, 0);
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcClearINTSts
Description: 
    clear a PU's interrupt sts. 
Input Param:
    U8 ucPU: PU number.
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to clear a PU's interrupt sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcClearINTSts(U8 ucPU)
{
    U8 ucCE;
    
    ucCE = HAL_NfcGetCE(ucPU);
    g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsIntSts = 1; // write 1, hardware clear it
#ifdef SIM
    g_pNfcCmdSts->aNfcqCmdStsReg[ucCE].bsIntSts = 0;
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetTriggerReg
Description: 
    set trigger reg according to CE, Lun, Wp and cmd request type.
Input Param:
    U8 ucCE: CE.
    U8 ucLun: Lun
    U8 ucWp: Wp
    U8 ucReqType: cmd request type
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set trigger reg when trigger a cmd request.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
void HAL_NfcSetTriggerReg(U8 ucCE, U8 ucLun, U8 ucWp, U8 ucReqType)
{
    NFC_TRIGGER_REG tNfcTriggerReg = {0};

    tNfcTriggerReg.bsPio = g_aPrcqTable[ucReqType].bsIsPIO;  
    tNfcTriggerReg.bsCmdType = g_aPrcqTable[ucReqType].bsTrigType;
        
#ifdef VT3514_C0
    if (0 != LUN_NUM_BITS)
    {
        //tNfcTriggerReg.bsLun = 0;
        //tNfcTriggerReg.bsLunEn = 0;
    }
    tNfcTriggerReg.bsInsert = HalNfcIsTrigInsert(ucReqType); 
    tNfcTriggerReg.bsCacheOp = HalNfcIsTrigCacheOp(ucReqType);
    *(volatile U16*)&g_pNfcTrigger->aNfcTriggerReg[ucCE][ucWp] = *(U16*)&tNfcTriggerReg;
#else
    tNfcTriggerReg.bsTrig = TRUE;
    *(volatile U8*)&g_pNfcTrigger->aNfcTriggerReg[ucCE][ucWp] = *(U8*)&tNfcTriggerReg;
#endif

#ifdef SIM
    NFC_SendCMD(ucCE);
#endif

    /* a few delay needed before access cmd sts register again */
    HAL_DelayCycle(CDC_WAIT_TIME);

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcCmdTrigger
Description: 
    trigger a cmd request accordint to flash addr and cmd request type.
Input Param:
    FLASH_ADDR tFlashAddr: flash addr.
    U8 ucReqType: cmd request type.
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set trigger a cmd request.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
void HAL_NfcCmdTrigger(FLASH_ADDR *pFlashAddr, U8 ucReqType)
{
    U8 ucCE;
    U8 ucLun;
    U8 ucWp;

    ucCE = HAL_NfcGetCE(pFlashAddr->ucPU);
    ucWp = HAL_NfcGetWP(pFlashAddr->ucPU);
    ucLun = HAL_NfcGetLUN(pFlashAddr->ucPU);

    HalNfcSetCmdType(ucCE, ucWp, ucReqType);
    HAL_NfcSetTriggerReg(ucCE, ucLun, ucWp, ucReqType);
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetRedAbsoulteAddr
Description: 
    get redundant relative addr according to PU number and it's Wp. 
    red-data-size = (RED_SZ << PLN_PER_PU_BITS)
Input Param:
    U8 ucPU: PU number
    U8 ucLevel: level number
    U8 ucPln: Pln number
Output Param:
    none
Return Value:
    U16: redundant physical addr.
Usage:
    FW call this function to get redundant physical addr for FW use.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20141030    Jason   modiy interface and implement.
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcGetRedAbsoulteAddr(U8 ucPU, U8 ucLevel, U8 ucPln)
{
    U8  ucRedOffset;
    U32 ulAbsoulteAddr;

    ucRedOffset = HAL_GET_SW_RED_ID(ucPU, ucLevel);

    ulAbsoulteAddr = ucPln * RED_SZ + ucRedOffset * (RED_SZ << PLN_PER_PU_BITS) + g_ulRedDataBase + RED_SZ_PER_MCU/2;

    return ulAbsoulteAddr;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetRedRelativeAddr
Description: 
    get redundant relative addr for HW use, 4DW align.
Input Param:
    U8 ucPU: PU number
    U8 ucLevel: level number
    U8 ucPln: Pln number
Output Param:
    none
Return Value:
    U16: redundant relative addr for HW use.
Usage:
    FW call this function to get redundant relative addr for HW use.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20141030    Jason   modiy interface and implement.
------------------------------------------------------------------------------*/
U16 MCU12_DRAM_TEXT HAL_NfcGetRedRelativeAddr(U8 ucPU, U8 ucLevel, U8 ucPln)
{
    U16 usRelativeAddr;
    U32 ulAbsoulteAddr;

    ulAbsoulteAddr = HAL_NfcGetRedAbsoulteAddr(ucPU, ucLevel, ucPln);

    usRelativeAddr = (U16)((ulAbsoulteAddr - OTFB_START_ADDRESS) >> 4);
    
    return usRelativeAddr;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcWaitStatus
Description: 
    Wait nfc idle and check error after a cmd triggered.
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    BOOL: TRUE: cmd request normal finished
          FALSE: cmd request caused error
Usage:
    FW call this function wait a cmd request to be done.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL HAL_NfcWaitStatus(U8 ucPU)
{
    while (FALSE == HAL_NfcGetIdle(ucPU))
    {
        #ifdef SIM
        NFC_ModelSchedule();
        #endif    
    }

    if (TRUE == HAL_NfcGetErrHold(ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    return NFC_STATUS_SUCCESS;
}


void MCU12_DRAM_TEXT HAL_NfcInitPU2LUN(void)
{
    U8 ucPU;
    U8 ucCE;
    U8 ucValidPUNum;

    ucValidPUNum = (MCU0_ID == HAL_GetMcuId()) ? CE_MAX : SUBSYSTEM_PU_NUM;

    for (ucPU = 0; ucPU < ucValidPUNum; ucPU++)
    {
        ucCE = HAL_NfcGetCE(ucPU);
        l_aPU2LUN[ucPU] = (ucCE/NFC_CH_TOTAL)%LUN_NUM;
    }

    for (; ucPU < CE_MAX; ucPU++)
    {
        l_aPU2LUN[ucPU] = INVALID_2F;
    }

    return;
}

U8 HAL_NfcGetLUN(U8 ucPU)
{
    return l_aPU2LUN[ucPU];
}


BOOL HAL_NfcIsPUAccessable(U8 ucPU, BOOL bCELevelType)
{
        if ((TRUE == HAL_NfcGetFull(ucPU)) || (TRUE == HAL_NfcGetErrHold(ucPU)))
        {
            return FALSE;
        }
        return TRUE;
}


/*------------------------------------------------------------------------------
Name: HAL_NfcResetFlash
Description: 
    build a reset flash cmd to one PU.
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    BOOL: TRUE: reset flash cmd build success
          FALSE: reset flash cmd build fail
Usage:
    FW call this function to build a reset flash cmd to one PU.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcResetFlash(U8 ucPU)
{
    NFCQ_ENTRY * pNFCQEntry;
    FLASH_ADDR tFlashAddr;
    tFlashAddr.ucPU = ucPU;
    
    if (TRUE != HAL_NfcIsPUAccessable(ucPU, TRUE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    HAL_NfcSetPRCQEntry(ucPU, NF_PRCQ_RESET);
    HAL_NfcCmdTrigger(&tFlashAddr, NF_PRCQ_RESET);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcReadId
Description: 
    read a PU's flash id.
Input Param:
    U8 ucPU: PU number
Output Param:
    U32 *pBuf: pointer to buf which store flash id.
Return Value:
    BOOL: TRUE: read id success
          FALSE: read id fail
Usage:
    FW call this function to read a PU's flash id.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcReadId(U8 ucPU, U32 *pBuf)
{
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr;
    tFlashAddr.ucPU = ucPU;

    if (TRUE != HAL_NfcIsPUAccessable(ucPU, TRUE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    pNFCQEntry->bsDmaByteEn = TRUE;
#ifdef TOGGLE20
    pNFCQEntry->bsByteAddr = 0x40;
#else
    pNFCQEntry->bsByteAddr = 0x00;
#endif
    pNFCQEntry->bsByteLength = 8;

    HAL_NfcSetPRCQEntry(ucPU, NF_PRCQ_READID);
    HAL_NfcCmdTrigger(&tFlashAddr,  NF_PRCQ_READID);
    
    while (FALSE == HAL_NfcGetIdle(ucPU));
    
    if (TRUE == HAL_NfcGetErrHold(ucPU))
    {
        DBG_Getch();
    }
    else
    {
        *pBuf = rNfcReadID0;
        *(pBuf + 1) = rNfcReadID1;
    }

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcPioSetFeature
Description: 
    build a pio set feature cmd to a PU. pio set feature means set flash feature by
    pio timing sequence.
Input Param:
    U8 ucPU: PU number
    U32 ulData: SetFeature data
    U8 ucAddr: SetFeature addr
Output Param:
    none
Return Value:
    BOOL: TRUE: build pio set feature success
          FALSE: build pio set feature fail
Usage:
    FW call this function to build a pio set feature cmd to a PU.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcPioSetFeature(U8 ucPU, U32 ulData, U8 ucAddr)
{
    U8 ucReqType;
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr;
    tFlashAddr.ucPU = ucPU;
    if (TRUE != HAL_NfcIsPUAccessable(ucPU, TRUE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    if (0x10 == ucAddr)
    {
        ucReqType = NF_PRCQ_PIO_SETFEATURE;
    }
    else if (0x80 == ucAddr)
    {
        ucReqType = NF_PRCQ_PIO_SETFEATURE_EX;
    }        
    else
    {
        DBG_Getch();
    }

    HAL_NfcSetPRCQEntry(ucPU, ucReqType);
    HAL_NfcCmdTrigger(&tFlashAddr, ucReqType);
    
    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetFeature
Description: 
    build a set feature cmd to a PU. set feature means set flash feature by
    normal timing sequence.
Input Param:
    U8 ucPU: PU number
    U32 ulData: SetFeature data
    U8 ucAddr: SetFeature addr
Output Param:
    none
Return Value:
    BOOL: TRUE: build set feature success
          FALSE: build set feature fail
Usage:
    FW call this function to build a set feature cmd to a PU.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSetFeature(U8 ucPU, U32 ulData, U8 ucAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr; 
    
    if (TRUE != HAL_NfcIsPUAccessable(ucPU, TRUE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsDmaByteEn = TRUE;
    rNfcSetFeature = ulData; 
    pNFCQEntry->bsByteAddr = ucAddr;
    pNFCQEntry->bsByteLength = sizeof(U32);

    tFlashAddr.ucPU = ucPU;
    HAL_NfcSetPRCQEntry(ucPU, NF_PRCQ_SETFEATURE);
    HAL_NfcCmdTrigger(&tFlashAddr, NF_PRCQ_SETFEATURE);

    return NFC_STATUS_SUCCESS;
}

void HAL_ConfigScramble(NFCQ_ENTRY *pNFCQEntry, U32 ulPage)
{
    /*enable scramble*/
    pNFCQEntry->bsPuEnpMsk = SCRAMBLE_MSK_EN;

    /*set scramble seed with page num*/
    *(U32 *)pNFCQEntry->bsScrambleSeed = ulPage;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcByteModeRead
Description: 
    read flash by byte mode
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U32 ulColumAddr: column address in flash page
    U8 ulLength: byte length
Output Param:
    U32 *pDataAddr: pointer to address of read data
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    used for some special opration to flash
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcByteModeRead(FLASH_ADDR *pFlashAddr, U32 ulColumAddr, U8 ulLength, NFC_RED **pDataAddr)
{
    NFCQ_ENTRY *pNFCQEntry;

    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsDmaByteEn = TRUE;
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);

    //set redundant addr, data read from flash will saved in red space
    pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);

    if (NULL != pDataAddr)
    {
        *pDataAddr = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
    }

    pNFCQEntry->aRowAddr[0]  = 0xffffff & HAL_NfcGetFlashRowAddr(pFlashAddr);
    pNFCQEntry->bsByteAddr   = ulColumAddr;
    pNFCQEntry->bsByteLength = ulLength;
    
    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_READ_1PLN);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_READ_1PLN);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSingleBlockErase
Description: 
    erase one block by single-plane.
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    "page" in pFlashAddr is not used, while "plane" should be specified clearly
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSingleBlockErase(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr;

    tFlashAddr = *pFlashAddr;
    if (TRUE != HAL_NfcIsPUAccessable(tFlashAddr.ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }

    tFlashAddr.usPage = 0;
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(tFlashAddr.ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    
    pNFCQEntry->aRowAddr[0] = 0xffffff & HAL_NfcGetFlashRowAddr(&tFlashAddr);
    HAL_NfcSetPRCQEntry(tFlashAddr.ucPU, NF_PRCQ_ERS_1PLN);
    HAL_NfcCmdTrigger(&tFlashAddr, NF_PRCQ_ERS_1PLN);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSinglePlnRead
Description: 
    build a single plane read cmd to a PU. the plane number is indicated 
    by FlashAddr Member ucPln.
Input Param:
    FlashAddr tFlashAddr: PU number
    U8 ucSecStart: sec start in one pln
    U8 ucSecLength: sec length
    BOOL bOTFBBuff: Buff(read out data locatio area) is ont OTFB flag
    U16 usBuffID: Buff Id    
Output Param:
    REDUNDANT **ppRedAddrOrg: pointer to redundant address
Return Value:
    BOOL: TRUE: build a single plane read cmd success
          FALSE: build a single plane read cmd fail
Usage:
    FW call this function to build a single plane read cmd to a PU.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlnRead(FLASH_ADDR *pFlashAddr,  U8 ucSecStart, U8 ucSecLength,
                                         BOOL bOTFBBuff, U16 usBuffID, NFC_RED **ppRedAddrOrg)
{
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY *pDSG;
    
    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry,sizeof(NFCQ_ENTRY)>>2);

    if(TRUE == bOTFBBuff)
    {
        pNFCQEntry->bsOntfEn = TRUE;
#ifdef VT3514_C0
        pNFCQEntry->bsOtfbBypass = TRUE;
#endif
    }
    
    pNFCQEntry->bsDsgEn = TRUE;
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    if (NULL != ppRedAddrOrg)
    {
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        *ppRedAddrOrg = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
    }

    pNFCQEntry->aRowAddr[0] = 0xffffff & HAL_NfcGetFlashRowAddr(pFlashAddr);
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);

    pNFCQEntry->aSecAddr[0].bsSecStart = ucSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = ucSecLength;
    pNFCQEntry->bsDmaTotalLength = ucSecLength;

    pDSG->ulDramAddr =  HAL_NfcGetDmaAddr(usBuffID, ucSecStart, (PIPE_PG_SZ_BITS - PLN_PER_PU_BITS));
    pDSG->bsXferByteLen = ucSecLength << SEC_SIZE_BITS ;
    pDSG->bsLast = TRUE;
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);
    
    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_READ_1PLN);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_READ_1PLN);
    
    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSinglePlaneWrite
Description: 
    write DRAM data to flash by single-plane
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U16 usBuffID: buffer id of DRAM
    NFC_RED *pRed: pointer to readundant data
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    usBuffID will be converted to DSG in this function.
    "plane" in pFlashAddr should be specified clearly
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
    20141113    tobey   fix bsRedEn problem
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlaneWrite(FLASH_ADDR *pFlashAddr, U16 usBuffID, NFC_RED *pRed)
{
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;    
    NFC_RED *pTargetRed;

    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);


    pNFCQEntry->bsDsgEn = TRUE;
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG, sizeof(NORMAL_DSG_ENTRY)>>2);
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    if (NULL != pRed)
    {
        pTargetRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        COM_MemCpy((U32 *)pTargetRed, (U32 *)pRed, sizeof(NFC_RED)/sizeof(U32)/PLN_PER_PU);
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        pNFCQEntry->bsRedEn = TRUE;
    }

    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);

    pNFCQEntry->aRowAddr[0] = 0xffffff & HAL_NfcGetFlashRowAddr(pFlashAddr);
    pNFCQEntry->aSecAddr[0].bsSecStart = 0;
    pNFCQEntry->aSecAddr[0].bsSecLength = SEC_PER_PG;

    pNFCQEntry->bsDmaTotalLength = SEC_PER_PG;

    pDSG->ulDramAddr = HAL_NfcGetDmaAddr(usBuffID, 0, (PIPE_PG_SZ_BITS - PLN_PER_PU_BITS));
    pDSG->bsXferByteLen = PG_SZ;
    pDSG->bsLast = TRUE;
    HAL_SetNormalDsgSts(usCurDsgId,1);
    
    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_PRG_1PLN);
    HAL_NfcCmdTrigger(pFlashAddr,NF_PRCQ_PRG_1PLN);

    return NFC_STATUS_SUCCESS;
}
/* end of file HAL_FlashDriverBasic.c */
