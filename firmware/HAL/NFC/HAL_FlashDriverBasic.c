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
20151105    abby      modify to meet VT3533
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_ParamTable.h"
#include "HAL_HostInterface.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_LdpcEngine.h"
#include "HAL_DecStsReport.h"
#ifdef SIM
#include "sim_flash_common.h"
#include "win_bootloader.h"
#endif

/*------------------------------------------------------------------------------
    VARIABLE DECLARATION
------------------------------------------------------------------------------*/
/* for run HAL_UnitTest on ASIC use by MPTool */
#ifdef ASIC
GLOBAL volatile U8 *g_pNfcSaveFwDone = (U8*)(DSRAM1_MCU012_SHARE_BASE + 0x800);
#endif

GLOBAL MCU12_VAR_ATTR volatile U32 *g_pNfcPRCQ;
GLOBAL MCU12_VAR_ATTR volatile NFCQ *g_pNfcq;
GLOBAL MCU12_VAR_ATTR volatile NFC_CMD_STS *g_pNfcCmdSts;
GLOBAL MCU12_VAR_ATTR volatile NFC_TRIGGER *g_pNfcTrigger;
GLOBAL MCU12_VAR_ATTR volatile NFC_DS_COLADDR *g_pNfcDsColAddr;
GLOBAL MCU12_VAR_ATTR volatile NFC_LOGIC_PU  *g_pNfcLogicPU;

GLOBAL volatile BOOL g_ErrInjEn;
GLOBAL NFC_ERR_INJ g_tErrInj = { 0 };

/*  RED  */
LOCAL MCU12_VAR_ATTR U32 g_ulRedDataOtfbBase;
LOCAL MCU12_VAR_ATTR U32 g_ulRedDataDramBase;
/*  PU mapping  */
LOCAL MCU12_VAR_ATTR U8 l_aPhyPUMap[NFC_PU_MAX];

/*  DS ENTRY in IRS  */
GLOBAL MCU12_DRAM_TEXT NFC_DS_ENTRY_REG g_aDsEntryTab[NF_DS_ENTRY_MAX] =
{
    /* bsRsv1:1 bsRedNum:3; bsLbaEn:1; bsNCRCEn:1; bsRsv2:3; bsCwNum:6; bsRsv3:17;    */
    {0, REDNUM_64BYTE, TRUE,     TRUE,     0, 16, 0, 0 },
    {0, REDNUM_64BYTE, FALSE,    TRUE,     0, 16, 0, 0 },
    {0, REDNUM_64BYTE, TRUE,     FALSE,    0, 16, 0, 0 },
    {0, REDNUM_64BYTE, FALSE,    FALSE,    0, 16, 0, 0 },
    /* 15K mode: not support LBA */
    {0, REDNUM_64BYTE, FALSE,    TRUE,     0, 15, 0, 0 },
    {0,  REDNUM_8BYTE, FALSE,    TRUE,     0, 15, 0, 0 },
    {0, REDNUM_64BYTE, FALSE,    FALSE,    0, 15, 0, 0 },
    {0,  REDNUM_8BYTE, FALSE,    FALSE,    0, 15, 0, 0 },
};


/*------------------------------------------------------------------------------
    EXTERNAL VARIABLE DECLARATION
------------------------------------------------------------------------------*/
extern MCU12_VAR_ATTR PRCQ_TABLE g_aPrcqTable[];

/*------------------------------------------------------------------------------
    LOCAL FUNCTION DECLARATION
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HAL_NfcSetRedDataBase(void);
LOCAL void MCU12_DRAM_TEXT HAL_NfcDSSramInit(void);

/*------------------------------------------------------------------------------
Name: HAL_NfcBuildPuMapping
Description:
    Initialize FW PU to physical PU mapping
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In FW and bootloader. call this funtion to Initialize FW PU to physical PU mapping
History:
    20140911    Tobey   create.
    20160210    abby    rmv logic PU mapping
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcBuildPuMapping(U32 ulBitMap)
{
    U8 ucPhyPU;
    U8 ucPU = 0;

    for (ucPhyPU = 0; ucPhyPU < NFC_PU_MAX; ucPhyPU++)
    {
        if (0 != (ulBitMap & (1 << ucPhyPU)))
        {
            l_aPhyPUMap[ucPU] = ucPhyPU;
            ucPU++;
        }
    }

    /*  PU without CE link will be invalid  */
    for (; ucPU < NFC_PU_MAX; ucPU++)
    {
        l_aPhyPUMap[ucPU] = INVALID_2F;
    }

    return;

}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetPhyPUBitMap
Description:
    Get the PhyPU BitMap of the current subsystem
Input Param:
    none
Output Param:
    none
Return Value:
    U32 ulSubSystemBitMap: PhyPU bitmap.
Usage:
    In FW and bootloader. call this funtion to get the PhyPU BitMap
History:
    20140911    Tobey   create.
    20141027    Jason   modify.
    20151118    abby    rmv MCU1 support, del input param ulMcuID
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcGetPhyPUBitMap(void)
{
    U32 ulSubSystemBitMap;

#ifdef COSIM
    ulSubSystemBitMap = 0xFFFFFFF; // custom redefined
#else
    ulSubSystemBitMap = HAL_GetSubSystemCEMap(MCU2_ID);// from param-table
#endif

    return ulSubSystemBitMap;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetLogicPUReg
Description:
    Initialize LogicPUReg according to two subsystem CE to LogicPU bit map.
Input Param:
    U32 ulPhyPUBitMap: physicalPU to LogicPU map for FW.
Output Param:
    none
Return Value:
    none
Usage:
    In BootLoader. call this funtion to Initialize LogicPU register for HW.
History:
    20140911    Tobey   create.
    20151111    abby    rmv MCU1 support
    20160308    abby    config invalid PU to avoid status report error
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcSetLogicPUReg(U32 ulPhyPUBitMap)
{
    U8 ucPhyPU;
    U8 ucLogicPU;

    /* set LogicPU register into a no map sts */
    ucLogicPU = 0;
    for (ucPhyPU = 0; ucPhyPU < NFC_PU_MAX; ucPhyPU++)
    {
        g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucPhyPU)][NFC_PU_IN_CH(ucPhyPU)].bsPuEnable = FALSE;
        g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucPhyPU)][NFC_PU_IN_CH(ucPhyPU)].bsLogicPU = ucLogicPU++;
    }

    ucLogicPU = 0;
    for (ucPhyPU = 0; ucPhyPU < NFC_PU_MAX; ucPhyPU++)
    {
        if (0 != (ulPhyPUBitMap & (1 << ucPhyPU)))
        {
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucPhyPU)][NFC_PU_IN_CH(ucPhyPU)].bsPuEnable = TRUE;
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucPhyPU)][NFC_PU_IN_CH(ucPhyPU)].bsLogicPU = ucLogicPU++;
        }
    }

    //invalid PU
    for (ucPhyPU = 0; ucPhyPU < NFC_PU_MAX; ucPhyPU++)
    {
        if (TRUE != (g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucPhyPU)][NFC_PU_IN_CH(ucPhyPU)].bsPuEnable))
        {
            g_pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucPhyPU)][NFC_PU_IN_CH(ucPhyPU)].bsLogicPU = ucLogicPU++;
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetRedDataBase
Description:
    Initialize global variable g_ulRedDataOtfbBase and g_ulRedDataDramBase.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In BootLoader and FW. call this funtion to init Red base addr.
History:
    20140911    Tobey   create.
    20151116    abby    rmv MCU1 support
                        add red dram base addr to meet VT3533
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HAL_NfcSetRedDataBase(void)
{
    volatile NF_RED_DRAM_BASE_REG *pRedBaseAddr;
    pRedBaseAddr = (volatile NF_RED_DRAM_BASE_REG *)&rNfcRedDramBase;

    g_ulRedDataDramBase = ((pRedBaseAddr->bsRedDramBaseAddr) << 3) + DRAM_START_ADDRESS;
    g_ulRedDataOtfbBase = OTFB_RED_MCU12_SHARE_BASE;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcBaseAddrInit
Description:
    Init NFC relative reg base address Initialization.
    Init PRCQ and NFCQ base address.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In bootloader/rom and FW.
    Called to init NFC global variables related with base address
History:
    20151116    abby    create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcBaseAddrInit(void)
{
    /* Data Syntax */
    g_pNfcDsColAddr = (NFC_DS_COLADDR *)DS_SRAM_BASE;

    /* PU Mapping */
    g_pNfcLogicPU   = (NFC_LOGIC_PU *)NF_LOGIC_PU_REG_BASE;

    /* NFCQ & DPTR */
    g_pNfcq         = (NFCQ*)NFCQ_ENTRY_BASE;
    g_pNfcCmdSts    = (NFC_CMD_STS*)NF_CMD_STS_REG_BASE;

    /* PRCQ */
    g_pNfcPRCQ      = (U32*)PRCQ_ENTRY_BASE;
    g_pNfcPRCQQe    = (NFC_PRCQ_QE*)NF_QE_REG_BASE;
    g_pNfcPRCQQeExt = (NFC_PRCQ_QE_EXT*)NF_QE_EXT_REG_BASE;

    /* NFC Trigger */
    g_pNfcTrigger   = (NFC_TRIGGER*)NF_TRIGGER_REG_BASE;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcInterfaceInitBasic
Description:
    Basic NFC relative base address Initialization.
    Init PRCQ related reg.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In bootloader and rom.Called to init NFC base addr and PRCQ
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20141024    Gavin   move Red pointer initialization to last step
    20151030    abby    modify to meet VT3533, add PRCQ table init
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcInterfaceInitBasic(void)
{
    /* NFC reg/PRCQ/NFCQ base addr init */
    HAL_NfcBaseAddrInit();

    /* PRCQ relative init */
    HAL_NfcQEEInit();
    HAL_NfcPrcqInit();
    HAL_NfcPrcqTableInit();

    /* Redundent */
    HAL_NfcSetRedInDramBase(DRAM_RED_MCU12_SHARE_BASE);
    HAL_NfcSetRedDataBase();

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSinglePlnReadSts
Description:
    read 1 plane status enhanced
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
Output Param:
    none
Return Value:
Usage:
History:
    20160811    abby    create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcSinglePlnReadSts(FLASH_ADDR *pFlashAddr)
{
    U8  ucPU, ucLun;
    NFCQ_ENTRY *pNFCQEntry;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(FLASH_PRCQ_READ_STS_ENHANCE);
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    HAL_NfcCmdTrigger(pFlashAddr, FLASH_PRCQ_READ_STS_ENHANCE, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}
/*------------------------------------------------------------------------------
Name: HAL_NfcGetDsSramEntryAddr
Description:
    Init NFC DS ENTRY in IRS
Input Param:
    U8 ucDSIndex: DS_ENTRY0-7;
    U8 ucCwIndex: 0-31;
Output Param:
    none
Return Value:
    NFC_DS_COLADDR_ENTRY*: CW DS info address in DS SRAM.
Usage:
    FW call this function in HAL_NfcDSSramInit.
History:
    20151228    abby    create
------------------------------------------------------------------------------*/
NFC_DS_COLADDR_ENTRY* HAL_NfcGetDsSramEntryAddr(U8 ucDSIndex, U8 ucCwIndex)
{
    return (NFC_DS_COLADDR_ENTRY *)(&g_pNfcDsColAddr->aDSColAddr[ucDSIndex][ucCwIndex]);
}


/*------------------------------------------------------------------------------
Name: HAL_NfcDSSramInit
Description:
    Init NFC DS_SRAM;
    Last 1K user data + red : use high-performance ECC code
Input Param:
    U8 ucDsIndex: DS entry index
    U8 ucCodeSel: LDPC matrix code index
Output Param:
    none
Return Value:
    none
Usage:
    This funtion is called by HAL_NfcDataSyntaxInit
History:
    20151225    abby    create
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HAL_NfcDSSramInit(void)
{
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    volatile NFC_DS_COLADDR_ENTRY *pDsSramEntry, *pPreDsSramEntry;
    U32 ulCwIndex = 0;
    U32 ulCwlength;
    U8  ucDsIndex;
    U8  ucCodeSel;

    /* Clear DS SRAM */
    COM_MemZero((U32*)g_pNfcDsColAddr, sizeof(NFC_DS_COLADDR)/sizeof(U32));

    for (ucDsIndex = DS_ENTRY0; ucDsIndex <= DS_ENTRY7; ucDsIndex++)
    {
        if (DS_15K_CRC == ucDsIndex)//for BootLoader
        {
            ucCodeSel = 0;
        }
        else
        {
            ucCodeSel = LDPC_MAT_SEL_FST_15K;
        }

        /*  init DW 0(bonding with CW 0 info) firstly   */
        pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, 0);
        if (FALSE == (pDsReg->atDSEntry[ucDsIndex].bsNCRCEn))
        {
            ulCwlength = NFC_DS_CW_INFO_LENTH >> 1;
        }
        else
        {
            ulCwlength = NFC_DS_CW_CRC_LENTH >> 1;
        }
        /* CW 0 */
        pDsSramEntry->bsCodeSel = ucCodeSel;
        pDsSramEntry->bsCwLenth = ulCwlength;
        pDsSramEntry->bsCwClAddr = 0;

        /* CW 1 - (CW_NUM-1) */
        for (ulCwIndex = 1; ulCwIndex < (pDsReg->atDSEntry[ucDsIndex].bsCwNum); ulCwIndex++)
        {
            pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex);
            pPreDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex - 1);

            pDsSramEntry->bsCodeSel = ucCodeSel;
            if ( (0 == (ulCwIndex + 1) % CW_PER_LBA) && (FALSE != (pDsReg->atDSEntry[ucDsIndex].bsLbaEn)) )
            {
                if (FALSE == (pDsReg->atDSEntry[ucDsIndex].bsNCRCEn))
                {
                    pDsSramEntry->bsCwLenth = NFC_DS_CW_LBA_LENTH >> 1;
                }
                else
                {
                    pDsSramEntry->bsCwLenth = NFC_DS_CW_CRC_LBA_LENTH >> 1;
                }
            }
            else
            {
                pDsSramEntry->bsCwLenth = ulCwlength;
            }
            pDsSramEntry->bsCwClAddr = pPreDsSramEntry->bsCwClAddr + pPreDsSramEntry->bsCwLenth + (g_atLdpcMatTable[LDPC_MAT_MODE][ucCodeSel] >> 1);
        }

        /* last CW */
        pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex - 1);
        if (DS_15K_CRC == ucDsIndex)//for BootLoader
        {
            ucCodeSel = 0;
        }
        else
        {
            ucCodeSel = LDPC_MAT_SEL_LASE_1K;
        }
        pDsSramEntry->bsCodeSel = ucCodeSel;
    }
    return;
}


/*------------------------------------------------------------------------------
Name: HAL_NfcDataSyntaxInit
Description:
    Init NFC DS REG
Input Param:
    U32 ulDsEntrySel: DS_ENTRY 0-7
    U8 ucCodeSel: LDPC matrix sel 0-3
Output Param:
    none
Return Value:
    none
Usage:
    This funtion should be called by bootloader or rom to init data syntax
History:
    20151118    abby    create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcDataSyntaxInit(void)
{
    U8  ucDsIndex = 0;
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;

    /* initial all DS entry, default use DS_ENTRY 0 */
    for (ucDsIndex = DS_ENTRY0; ucDsIndex <= DS_ENTRY7; ucDsIndex++)
    {
        pDsReg->atDSEntry[ucDsIndex].bsRedNum = g_aDsEntryTab[ucDsIndex].bsRedNum;
        pDsReg->atDSEntry[ucDsIndex].bsNCRCEn = g_aDsEntryTab[ucDsIndex].bsNCRCEn;
        pDsReg->atDSEntry[ucDsIndex].bsLbaEn  = g_aDsEntryTab[ucDsIndex].bsLbaEn;
        pDsReg->atDSEntry[ucDsIndex].bsCwNum  = g_aDsEntryTab[ucDsIndex].bsCwNum;
    }

    /* initial DS column address info in SRAM */
    HAL_NfcDSSramInit();

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetPhyPU
Description:
    Get a PU's corresponding physical PU number.
Input Param:
    ucPU: PU num
Output Param:
    none
Return Value:
    U8: PhyPU number.
Usage:
    FW call this function if it wants obtain a PU's corresponding CE number.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151109    abby    change CE to physical PU
------------------------------------------------------------------------------*/
U8 HAL_NfcGetPhyPU(U8 ucPU)
{
    return l_aPhyPUMap[ucPU];
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetWP
Description:
    Get a PU's write pointer.
Input Param:
    U8 ucPU: PU number.
    U8 ucLun: LUN number.
Output Param:
    none
Return Value:
    U8: PU->LUN write pointer
Usage:
    FW call this function if it wants obtain a PU's write pointer.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151028     abby    add parameter ucLun
------------------------------------------------------------------------------*/
U8 HAL_NfcGetWP(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU;
    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    return (U8)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsWrPtr;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetRP
Description:
    Get a PU's read pointer.
Input Param:
    U8 ucPU: PU number.
    U8 ucLun: LUN
Output Param:
    none
Return Value:
    U8: PU's read pointer
Usage:
    FW call this function if it wants obtain a PU's read pointer.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151029    abby     modify to meet VT3533
------------------------------------------------------------------------------*/
U8 HAL_NfcGetRP(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU;
    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    return (U8)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsRdPtr;
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
BOOL HAL_NfcGetIdle(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU;

    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsIdle;
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
BOOL HAL_NfcGetFull(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU;
    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsFull;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetEmpty
Description:
    Get a PU's empty sts. HW set empty when all HW cmd queue not filled.
Input Param:
    U8 ucPU: PU number.
    U8 ucLun: LUN
Output Param:
    none
Return Value:
    BOOL: 1: empty
          0: not empty
Usage:
    FW call this function if it wants obtain a PU's empty sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151029    abby    add ucLun para
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetEmpty(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU;
    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsEmpty;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetErrHold
Description:
    Get a PU's error sts. HW set bsErrh when error occur.
Input Param:
    U8 ucPU: PU number.
    U8 ucLun: LUN
Output Param:
    none
Return Value:
    BOOL: 1: error hold
          0: no error
Usage:
    FW call this function if it wants obtain a PU's error sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151030    abby    add LUN as para
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetErrHold(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU;
    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    return (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsErrh;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetErrCode
Description:
    get PU error status.
Input Param:
    U8 ucPU: PU number
    U8 ucLun: Lun
Output Param:
    none
Return Value:
    U8: error status.
Usage:
    anywhere need check NFC error status.
History:
    20140912    Gavin   modify to meet coding style
    20151030    abby    add LUN as para
------------------------------------------------------------------------------*/
U8 HAL_NfcGetErrCode(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU;
    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    return (U8)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsErrSts;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetFinish
Description:
    get PU one level's finish sts.
Input Param:
    U8 ucPU: PU number
    U8 ucLun: LUN
    U8 ucLevel: HW queue num
Output Param:
    none
Return Value:
    BOOL:TURE: finished; FALSE:not finished;
Usage:
    anywhere need check NFC error status.
History:
    20140912    Gavin   modify to meet coding style
    20151030    abby    add LUN
------------------------------------------------------------------------------*/
BOOL HAL_NfcGetFinish(U8 ucPU, U8 ucLun, U8 ucLevel)
{
    U8 ucPhyPU;
    BOOL bFinishFlag = FALSE;

    ucPhyPU = HAL_NfcGetPhyPU(ucPU);

    if (0 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsFsLv0;
    }
    else if (1 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsFsLv1;
    }
    else if (2 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsFsLv2;
    }
    else if(3 == ucLevel)
    {
        bFinishFlag = (BOOL)g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsFsLv3;
    }
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
    20160116    abby    modify it to meet TLC driver request
------------------------------------------------------------------------------*/
U32 HAL_NfcGetDmaAddr(U32 ulBuffID, U32 ulSecInBuff, U32 ulBufSizeBits)
{
    U32 ulAddr;

    ulAddr = ((ulBuffID << ulBufSizeBits) + (ulSecInBuff << SEC_SZ_BITS));

    return (ulAddr >> 1);

}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetNfcqEntryAddr
Description:
    Get a PU's current Wp directing NFCQ entry address.
    caution, NFCQ entry is ordered by LogicPU.
Input Param:
    U8 ucPU: PU number
    U8 ucLun: LUN number
Output Param:
    none
Return Value:
    U32: NFCQ entry address.
Usage:
    FW call this function if it wants obtain a PU's current Wp directing NFCQ entry address.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151028    abby    add para ucLun,del para RP
------------------------------------------------------------------------------*/
NFCQ_ENTRY * HAL_NfcGetNfcqEntryAddr(U8 ucPU, U8 ucLun)
{
    U8 ucLevel;
    ucLevel = HAL_NfcGetWP(ucPU, ucLun);

    return (NFCQ_ENTRY *)(&g_pNfcq->aNfcqEntry[ucPU][ucLun][ucLevel]);
}

NFCQ_ENTRY * HAL_NfcGetNfcqEntryAddr_RP(U8 ucPU, U8 ucLun)
{
    U8 ucLevel;
    ucLevel = HAL_NfcGetRP(ucPU, ucLun);

    #ifdef FLASH_CACHE_OPERATION
    U8 ucErrCode;

    ucErrCode = HAL_NfcGetErrCode(ucPU, ucLun);
    if (NF_ERR_TYPE_PREPRG == ucErrCode || NF_ERR_TYPE_BOTHPRG == ucErrCode)
    {
        ucLevel = (NFCQ_DEPTH + ucLevel - 1) % NFCQ_DEPTH;
    }
    #endif

    return (NFCQ_ENTRY *)(&g_pNfcq->aNfcqEntry[ucPU][ucLun][ucLevel]);
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetPrcqEntryAddr
Description:
    Get a cmd PRCQ address
Input Param:
    U8 ucReqType: cmd type
Output Param:
    none
Return Value:
    U32: PRCQ address.
Usage:
    FW call this function if it wants obtain PRCQ entry address.Typically use in retry.
History:
    20160115    abby    create
------------------------------------------------------------------------------*/
U32 HAL_NfcGetPrcqEntryAddr(U8 ucReqType)
{
    U32 ulAddrOffset;
    ulAddrOffset = HAL_NfcGetPrcqStartDw(ucReqType) << 2; //DW align to byte align

    return (U32)(PRCQ_ENTRY_BASE + ulAddrOffset);
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
    20151109    abby    modify ucLUN calculation to support CE decode
------------------------------------------------------------------------------*/
U32 HAL_NfcGetFlashRowAddr(FLASH_ADDR * pFlashAddr)
{
    U8 ucLunInCE, ucPln;
    U16 usPage, usBlk;
    U32 addr;

    ucPln = pFlashAddr->bsPln;
    usBlk = pFlashAddr->usBlock;
    usPage = pFlashAddr->usPage;

    /*  ucPhyLUN = ucLun when CE decode is disable, otherwise ucLun = ucPhyLUN * CE_sel*/
    ucLunInCE = pFlashAddr->ucLun & LUN_PER_CE_MSK;
    if ((pFlashAddr->bsSLCMode)&&(pFlashAddr->bsSLCShiftAddrByFW)) // no shift in flash, we shift it by FW
    {
        addr = HAL_FLASH_SLC_ROW_ADDR(ucLunInCE, ucPln, usBlk, usPage);
    }
    else
    {
        addr = HAL_FLASH_ROW_ADDR(ucLunInCE, ucPln, usBlk, usPage);
    }

    if (usPage < 2)
        ;//DBG_Printf("PLN%d BLK%d Page%d Row Addr = 0x%x\n", ucPln, usBlk, usPage, addr);

    return addr;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcResetCmdQue
Description:
    reset a PU's HW cmd queue into a avialble sts.
Input Param:
    U8 ucPU: PU number.
    U8 ucLun: LUN
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
    20151029     abby    add Lun
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcResetCmdQue(U8 ucPU, U8 ucLun)
{
    U8 ucPhyPU = HAL_NfcGetPhyPU(ucPU);

#ifdef SIM
    NFCM_LUN_LOCATION tLunLocation;

    tLunLocation.ucCh = ucPhyPU % NFC_CH_TOTAL;
    tLunLocation.ucPhyPuInCh = ucPhyPU / NFC_CH_TOTAL;
    tLunLocation.ucLunInPhyPu = ucLun;

    NFC_InterfaceResetCQ(&tLunLocation, 0);
#else
    g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPU][ucLun].bsPrst = TRUE;
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcClearINTSts
Description:
    clear a PU's interrupt sts.
Input Param:
    U8 ucPU: PU number.
    U8 ucLun: LUN
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to clear a PU's interrupt sts.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151030    abby    add ucLun as para
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcClearINTSts(U8 ucPU, U8 ucLun)
{
#ifndef SIM
    U8 ucPhyPuInTotal = HAL_NfcGetPhyPU(ucPU);
    g_pNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLun].bsIntSts = 1; // write 1, hardware clear it
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcCmdTrigger
Description:
    trigger a cmd request accordint to flash addr and cmd request type.
Input Param:
    FLASH_ADDR tFlashAddr: flash addr.
    U8 ucReqType: cmd request type.
    NFC_XOR_PARITY XorParityInData: Only need by write command, use to tell NFC
    there is XOR parity inside this NFC command's data or not, if have parity,
    the parity is from which XOR engine.
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set trigger a cmd request.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151030    abby    modify to meet VT3533, add input para ucXorEngId
    20170122    Jason   update the code sytle.
------------------------------------------------------------------------------*/
void HAL_NfcCmdTrigger(FLASH_ADDR *pFlashAddr, U8 ucReqType, BOOL bIsXorParityWriteCmd, U8 ucXorId)
{
    U8 ucPhyPU, ucLun, ucWp;
    NFC_TRIGGER_REG tNfcTriggerReg = {0};

    ucLun   = pFlashAddr->ucLun;
    ucPhyPU = HAL_NfcGetPhyPU(pFlashAddr->ucPU);
    ucWp    = HAL_NfcGetWP(pFlashAddr->ucPU, ucLun);

    #if (defined(XTMP) || defined(SIM))
    NfcM_ISetCmdCode(ucPhyPU, ucLun, ucWp, ucReqType);
    NfcM_ISetCmdMode(ucPhyPU, ucLun, ucWp, pFlashAddr->bsSLCMode);
    #endif

    tNfcTriggerReg.bsCmdType = g_aPrcqTable[ucReqType].bsTrigType;
    #ifdef FLASH_CACHE_OPERATION
    tNfcTriggerReg.bsCacheOp = HAL_FlashIsTrigCacheOp(ucReqType);
    #endif

    /* CE_SEL = 1 only when CE decode enable & Logic LUN = 2/3 & LUN_PER_CE = 2 */
    tNfcTriggerReg.bsCESel = (0 == NFC_CE_PER_PU_BITS) ? 0 : (ucLun >> LUN_PER_CE_BITS);

    /* For write command, 1-stand for write XOR parity  */
#ifdef XOR_ENABLE
    if ((TRUE == bIsXorParityWriteCmd) && (NFC_WRITE_CMD == tNfcTriggerReg.bsCmdType))
    {
        tNfcTriggerReg.bsExtCmd = TRUE;
        tNfcTriggerReg.bsExtCmdSel = ucXorId;
    }
    else
#endif
    /* for cache read command, 1-stand for single plane cache */
#ifdef FLASH_CACHE_OPERATION
    if ((NFC_READ_CMD == tNfcTriggerReg.bsCmdType) && (TRUE == HAL_FlashIs1PlnOp(ucReqType)))
    {
        tNfcTriggerReg.bsExtCmd = TRUE;
        tNfcTriggerReg.bsExtCmdSel = pFlashAddr->bsPln;
    }
    else
#endif
    /* For Normal or PIO cmd: 0-Normal; 1-PIO  */
    {
        if (ucReqType >= FLASH_PRCQ_NON_PIO_NUM)
        {
            tNfcTriggerReg.bsExtCmd = 1;//g_aPrcqTable[ucReqType].bsIsPioCmd;
        }
    }

    g_pNfcTrigger->aNfcTriggerReg[ucPhyPU][ucLun][ucWp].ucTrigValue = tNfcTriggerReg.ucTrigValue;

#ifdef SIM
    NFC_SendCMD(ucPhyPU, ucLun);
    NFC_ModelSchedule();
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetRedAbsoulteAddr
Description:
    get redundant relative addr according to PU->LUN->PLN->WP
Input Param:
    U8 ucPU: PU number
    U8 ucLun: LUN num
    U8 ucWp: cmd level
    U8 ucPln: Pln number
    BOOL bIsOntf: 1-OTFB, 0-DRAM
Output Param:
    none
Return Value:
    U16: redundant physical addr.
Usage:
    FW call this function to get redundant physical addr for FW use.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20141030    Jason   modiy interface and implement.
    20151105    abby    modify to meet VT3533,dynamic update to OTFB or DRAM, and support multi-lun
------------------------------------------------------------------------------*/
U32 HAL_NfcGetRedAbsoulteAddr(U8 ucPU, U8 ucLun, U8 ucWp, U8 ucPln, BOOL bIsOntf)
{
    U32 ulRedOffset;
    U32 ulAbsoulteAddr;
    U32 ulRedBaseAddr;

    ulRedBaseAddr = (FALSE == bIsOntf) ? g_ulRedDataDramBase : g_ulRedDataOtfbBase;

    ulRedOffset = HAL_GET_SW_RED_ID(ucPU, ucLun, ucPln, ucWp);

    ulAbsoulteAddr = ulRedOffset * RED_SZ + ulRedBaseAddr;

    return ulAbsoulteAddr;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetRedRelativeAddr
Description:
    get redundant relative addr for HW use, 4DW align.
Input Param:
    U8 ucPU: PU number
    U8 ucLun: LUN number
    U8 ucWp: cmd level
    U8 ucPln: Pln number
    BOOL bIsOntf: 1-OTFB, 0-DRAM
Output Param:
    none
Return Value:
    U16: redundant relative addr for HW use.
Usage:
    FW call this function to get redundant relative addr for HW use.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20141030    Jason   modiy interface and implement.
    20151105    abby    modify to meet VT3533, dynamic update to OTFB or DRAM
------------------------------------------------------------------------------*/
U32 HAL_NfcGetRedRelativeAddr(U8 ucPU, U8 ucLun, U8 ucWp, U8 ucPln, BOOL bIsOntf)
{
    U32 ulRelativeAddr;
    U32 ulAbsoulteAddr;
    U32 ulStartAddr;

    ulStartAddr = (FALSE == bIsOntf) ? g_ulRedDataDramBase : OTFB_START_ADDRESS;

    ulAbsoulteAddr = HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun, ucWp, ucPln, bIsOntf);

    /* 8Byte Align    */
    ulRelativeAddr = (ulAbsoulteAddr - ulStartAddr) >> 3;

    return ulRelativeAddr;
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
BOOL HAL_NfcWaitStatus(U8 ucPU, U8 ucLun)
{
    while (FALSE == HAL_NfcGetIdle(ucPU, ucLun))
    {
        ;
    }

    if (TRUE == HAL_NfcGetErrHold(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    return NFC_STATUS_SUCCESS;
}


/*------------------------------------------------------------------------------
Name: HAL_NfcIsLunAccessable
Description:
    Wait nfc idle and check error after a cmd triggered.
Input Param:
    U8 ucPU: PU number
    U8 ucLun:LUN number
Output Param:
    none
Return Value:
    BOOL: TRUE: cmd request normal finished
          FALSE: cmd request caused error
Usage:
    FW call this function wait a cmd request to be done.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151105    abby    expend to lun level, change name from HAL_NfcIsPUAccessable
-----------------------------------------------------------------------------*/
BOOL HAL_NfcIsLunAccessable(U8 ucPU, U8 ucLun)
{
    if ((TRUE == HAL_NfcGetFull(ucPU, ucLun)) || (TRUE == HAL_NfcGetErrHold(ucPU, ucLun)))
    {
        return FALSE;
    }
    return TRUE;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcIsPairPageAccessable
Description:
    For 3D MLC flash, 1-pass program shouldn't start from HIGH_PAGE
Input Param:
    U16 usPage: page number
Output Param:
    none
Return Value:
    FALSE: can't be program
Usage:
    FW call this function check if the page can be program.
History:
    20160416    abby    create
-----------------------------------------------------------------------------*/
BOOL HAL_NfcIsPairPageAccessable(U16 usPage)
{
    /* only FLASH_3D_MLC and FLASH_INTEL_3DTLC program need to check */
    #ifndef FLASH_3D_MLC
        return TRUE;
    #endif

    PAIR_PAGE_TYPE ePagType;
    ePagType = HAL_GetFlashPairPageType(usPage);
    if (HIGH_PAGE == ePagType)
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
    FLASH_ADDR *pFlashAddr: flash address
Output Param:
    none
Return Value:
    BOOL: TRUE: reset flash cmd build success
          FALSE: reset flash cmd build fail
Usage:
    FW call this function to build a reset flash cmd to one PU.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151212    abby    modified to meet VT3533
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcResetFlash(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY * pNFCQEntry;
    U8 ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    // if reset PU, whatever the LUN is; but if reset LUN, the LUN num is meanful
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_RESET);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_RESET, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcResetFlashNoWaitStatus
Description:
    build a reset but not wait status cmd to patch non-valid flash will be not
    idle when read status
Input Param:
    FLASH_ADDR *pFlashAddr: flash address
Output Param:
    none
Return Value:
    BOOL: TRUE: reset flash cmd build success
          FALSE: reset flash cmd build fail
Usage:
    FW call this function to build a reset flash cmd to one PU.
History:
    20160901    abby    create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcResetFlashNoWaitStatus(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY * pNFCQEntry;
    U8 ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    // if reset PU, whatever the LUN is; but if reset LUN, the LUN num is meanful
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_RESET_NOSTS);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_RESET_NOSTS, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcResetLun
Description:
    build a reset LUN cmd to one LUN.
Input Param:
    FLASH_ADDR *pFlashAddr: flash address
Output Param:
    none
Return Value:
    BOOL: TRUE: success
          FALSE: fail
Usage:
    FW call this function to reset LUN in cache read error handle
History:
    20150223    abby   create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcResetLun(FLASH_ADDR *pFlashAddr)
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

    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_RESET_LUN);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_RESET_LUN, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSyncResetFlash
Description:
    build a sync reset flash cmd to one PU.
Input Param:
    FLASH_ADDR *pFlashAddr: a pointer to flash addr
Output Param:
    none
Return Value:
    BOOL: TRUE: reset flash cmd build success
          FALSE: reset flash cmd build fail
Usage:
    FW call this function to init 3D MLC after change trim file.
History:
    20160329    abby    create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSyncResetFlash(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY * pNFCQEntry;
    U8 ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    // if reset PU, whatever the LUN is; but if reset LUN, the LUN num is meanful
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_SYNC_RESET);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_SYNC_RESET, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

BOOL MCU12_DRAM_TEXT HAL_NfcHardResetFlash(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY * pNFCQEntry;
    U8 ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    // if reset PU, whatever the LUN is; but if reset LUN, the LUN num is meanful
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_HARD_RESET);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_HARD_RESET, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcReadFlashId
Description:
    read a PU's flash id.
Input Param:
    U8 ucPU: PU number
Output Param:
    none
Return Value:
    BOOL: TRUE: read id success
          FALSE: read id fail
Usage:
    FW call this function to read a PU's flash id into DEC_SRAM
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151109    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcReadFlashId(FLASH_ADDR * pFlashAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    U8 ucPU, ucLun, ucWp;
    U32 *pTarID;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    ucWp = HAL_NfcGetWP(ucPU, ucLun);

    // don't care LUN, reset as PU
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    //clr ID SRAM
    pTarID = (U32*)(&(g_pDecSramStsBase->aFlashId[ucPU][ucLun][ucWp][0]));
    COM_MemZero(pTarID, sizeof(FLASH_ID_ENTRY)>>2);

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsDmaByteEn = TRUE;
    pNFCQEntry->aByteAddr.usByteAddr = 0x00;
    pNFCQEntry->aByteAddr.usByteLength = 8;

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_READID);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_READID, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcPioSetFeature
Description:
    build a pio set feature cmd to a PU. pio set feature means set flash feature by
    pio timing sequence.
Input Param:
    U8 ucPU: PU number
    U8 ucLun: LUN number
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
    20151109    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcPioSetFeature(U8 ucPU, U8 ucLun, U32 ulData, U8 ucAddr)
{
    U8 ucReqType;
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr = { 0 };

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
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

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    HAL_NfcCmdTrigger(&tFlashAddr, ucReqType, FALSE, INVALID_2F);

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
    20151210    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSetFeature(FLASH_ADDR *pFlashAddr, U32 ulData, U8 ucAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    U8 ucPU, ucLun;

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

    //if (ucAddr == 0x91)//dannier set to nonScrach addressing mode
      //  pNFCQEntry->ulSetFeatVal = 0x100 | ulData;
    //else
        pNFCQEntry->ulSetFeatVal = ulData;

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_SETFEATURE);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_SETFEATURE, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

BOOL MCU12_DRAM_TEXT HAL_NfcGetFeature(FLASH_ADDR *pFlashAddr, U8 ucAddr)
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
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_GETFEATURE);

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_GETFEATURE, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_ConfigScramble
Description:
    Enable scramble and sel the scramble seed index.
    For MLC: use page num as scr seed;
    For TLC: use different seeds for different types of pages in one WL
Input Param:
    NFCQ_ENTRY *pNFCQEntry: NFCQ of current cmd
    U8 ulPage: page num, only low 8 bits valid
    U8 ucTlcPlnNum: plane number share the same seed;
                    0-not TLC op; 1/2/3: 2/4/8 pln
    NFC_SCR_CONF eScrType: define different cmd type to config sramble
Output Param:
    none
Return Value:
    none
Usage:
    Called in FW when W/R data into flash with srcamble
History:
20150629   Jason   modify
20150709   Jason   use deferent seeds between pages in a block, according abbin's solution.
20160411   Abby    add 3D flash support
------------------------------------------------------------------------------*/
void HAL_ConfigScramble(NFCQ_ENTRY *pNFCQEntry, U16 usPage, U8 ucPlnNum, U8 ucPageType, NFC_SCR_CONF eScrType)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;

    /*enable scramble*/
    pNFCQEntry->bsPuEnpMsk = SCRAMBLE_MSK_EN;
    U16 usSeed;

    /*set scramble seed with page num & plane only when scramble is enable */
    if (FALSE != pNFCQEntry->bsPuEnpMsk)
    {
        return;
    }

    switch (eScrType)
    {
        case NORMAL_SCR :
        {
            /* set scramble seed index as page num when scramble is enable */
            pNFCQEntry->bsScrSeed0 = usPage;//usPage - (usPage%2);//HAL_FlashGetScrSeed(usPage, eScrType, ucPageType);
        }break;

        case TLC_WT_THREE_PG :
        {
            pNFCQEntry->bsScrSeed0 = HAL_FlashGetScrSeed(usPage, eScrType, 0);

            usSeed = HAL_FlashGetScrSeed(usPage, eScrType, 1);
            pNFCQEntry->bsScrSeed1Lsb = usSeed & 0x1;            //bit0
            pNFCQEntry->bsScrSeed1Msb = (usSeed & 0xFE) >> 1;    //bit7-1

            usSeed = HAL_FlashGetScrSeed(usPage, eScrType, 2);
            pNFCQEntry->bsScrSeed2Lsb = usSeed & 0x1;            //bit0
            pNFCQEntry->bsScrSeed2Msb = (usSeed & 0xFE) >> 1;    //bit7-1
        }break;

        case TLC_RD_ONE_PG :
        {
            pNFCQEntry->bsScrSeed0 = HAL_FlashGetScrSeed(usPage, eScrType, ucPageType);
        }break;

        case MLC_RW_TWO_PG :
        {
            pNFCQEntry->bsScrSeed0 = HAL_FlashGetScrSeed(usPage, eScrType, ucPageType);

            usSeed = HAL_FlashGetScrSeed(usPage + 1, eScrType, ucPageType);
            pNFCQEntry->bsScrSeed1Lsb = usSeed & 0x1;            //bit0
            pNFCQEntry->bsScrSeed1Msb = (usSeed & 0xFE) >> 1;    //bit7-1
        }break;

    #ifdef FLASH_INTEL_3DTLC
        case TLC_RW_TWO_PG :
        {
            pNFCQEntry->bsScrSeed0 = HAL_FlashGetScrSeed(usPage, eScrType, 0);

            if(EXTRA_PAGE == HAL_GetFlashPairPageType(usPage))
            {
#ifdef FLASH_IM_3DTLC_GEN2
                usSeed = HAL_FlashGetScrSeed(usPage + 1, eScrType, 0);
#else
                usSeed = HAL_FlashGetScrSeed(HAL_GetHighPageIndexfromExtra(usPage), eScrType, 0);
#endif
                pNFCQEntry->bsScrSeed1Lsb = usSeed & 0x1;            //bit0
                pNFCQEntry->bsScrSeed1Msb = (usSeed & 0xFE) >> 1;     //bit7-1
            }
            else
            {
                usSeed = HAL_FlashGetScrSeed(usPage + 1, eScrType, 0);
                pNFCQEntry->bsScrSeed1Lsb = usSeed & 0x1;            //bit0
                pNFCQEntry->bsScrSeed1Msb = (usSeed & 0xFE) >> 1;    //bit7-1
            }
        }break;
    #endif
        default :
        {
            DBG_Printf("Scramble configure type error!\n");
            DBG_Getch();
        }
    }

    /* config PLN number for TLC write or 3D MLC  */
    if (TRUE == pNfcPgCfg->bsRTSB3dTlc)
    {
        pNFCQEntry->bsTlcPlnNum = ucPlnNum;
    }
    //U8 ucSeed1= (pNFCQEntry->bsScrSeed1Msb<<1) | pNFCQEntry->bsScrSeed1Lsb;
    //if (usPage < 4)
        //DBG_Printf("Page%d SCR Seed0=%d Seed1=%d\n", usPage, pNFCQEntry->bsScrSeed0, ucSeed1);
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetErrInj
Description:
    Err injection setting NFCQ
Input Param:
Output Param:
none
Return Value:
Usage:
    Only for read operation
History:
    20160317   abby    create
------------------------------------------------------------------------------*/
void HAL_NfcSetErrInj(NFCQ_ENTRY *pNFCQEntry, NFC_ERR_INJ *pErrInj)
{
    pNFCQEntry->bsInjEn = TRUE;

    /* For erase or write: use ucErrSts to replace the last read sts value */
    pNFCQEntry->bsInjStsVal  = pErrInj->bsInjErrSts;

    /* For read: inject error bit */
    pNFCQEntry->bsInjCwStart = pErrInj->bsInjCwStart;
    pNFCQEntry->bsInjCwCnt   = pErrInj->bsInjCwLen;
    pNFCQEntry->bsInjErrCnt  = pErrInj->bsInjErrBitPerCw;

    /* For all types of flash operation */
    pNFCQEntry->bsErrTypeS   = pErrInj->bsInjErrType;

    return;
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
    20151029    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSingleBlockErase(FLASH_ADDR *pFlashAddr, BOOL bTlcMode)
{
    U8 ucPU, ucLun, ucReqType;
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr = { 0 };

    tFlashAddr = *pFlashAddr;
    /* force page = 0 in erase to avoid flash abnormal response */
    tFlashAddr.usPage = 0;
    ucPU = tFlashAddr.ucPU;
    ucLun = tFlashAddr.ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (FALSE == bTlcMode)
    {
        ucReqType = NF_PRCQ_ERS;
    }
    else
    {
        ucReqType = NF_PRCQ_MLC_ERS;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    
#ifdef HAL_NFC_TEST
    if(FALSE != g_ErrInjEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, &g_tErrInj);
    }
#endif

    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    HAL_NfcCmdTrigger(&tFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSinglePlnRead
Description:
    build a single plane read cmd to a PU. the plane number is indicated
    by FlashAddr Member ucPln.
Input Param:
    FlashAddr tFlashAddr: PU number
    NFC_READ_REQ_DES *pRdReq;
    BOOL bOTFBBuff: Buff(read out data locatio area) is ont OTFB flag
Output Param:
    REDUNDANT **ppRedAddrOrg: pointer to redundant address
Return Value:
    BOOL: TRUE: build a single plane read cmd success
          FALSE: build a single plane read cmd fail
Usage:
    FW call this function to build a single plane read cmd to a PU.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151105    abby    modify to meet VT3533, dynamic update RED to OTFB/DRAM
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlnRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, BOOL bOTFBBuff)
{
    U8  ucPU, ucLun, ucPln, ucReqType;
    U8  ucPageType = 0;
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY *pDSG;
    NFC_SCR_CONF eScrType = NORMAL_SCR;
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    U8  ucDsIndex = pRdReq->bsDsIndex;
    U8  ucBufUnit = ((pDsReg->atDSEntry[ucDsIndex].bsCwNum % 16) ? CW_INFO_SZ_BITS : LOGIC_PG_SZ_BITS);


    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (pRdReq->bsTlcMode)//MLC
    {
        ucPageType = 0;
        ucReqType  = NF_PRCQ_MLC_READ;
        eScrType   = NORMAL_SCR;
        ucPln      = pFlashAddr->bsPln;
    }
    else
    {
        ucPageType = 0;
        ucReqType  = NF_PRCQ_READ;
        eScrType   = NORMAL_SCR;
        ucPln = pFlashAddr->bsPln;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry,sizeof(NFCQ_ENTRY)>>2);

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = ucDsIndex;  //select flash page layout by DS entry in IRS
    pNFCQEntry->bsEMEn    = pRdReq->bsEmEn;

    /*  DSG configure   */
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsDsgEn = TRUE;
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  OTFB configure  */
    if(FALSE != bOTFBBuff)
    {
        pNFCQEntry->bsOntfEn   = TRUE;
        pNFCQEntry->bsTrigOmEn = TRUE;
    }

    /*  NFCQ DW1: sector enable and length   */
    pNFCQEntry->aSecAddr[0].bsSecStart = pRdReq->bsSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = pRdReq->bsSecLen;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pRdReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pRdReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    pNFCQEntry->bsDmaTotalLength = pRdReq->bsSecLen;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pRdReq->ppNfcRed)
    {
        pNFCQEntry->bsRedOntf = pRdReq->bsRedOntf;
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,pRdReq->bsRedOntf);
        *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, pRdReq->bsRedOntf);
    }
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_SINGLE_PLN, ucPageType, eScrType);

    /*  NFCQ DW5: LBA for read, fill by FW, the same with corresponding LBA in write red   */
    if (pRdReq->bsLbaChkEn || pRdReq->bsEmEn)
    {
        pNFCQEntry->bsRdLba = pRdReq->bsLba;
    }
    
    /* Raw data read = true, each CW 2K align */
    pNFCQEntry->bsRawReadEn = pRdReq->bsRawData;

    /* DEC FIFO status report config */
    if (FALSE != pRdReq->bsDecFifoEn)
    {
        HAL_DecFifoTrigNfc(pNFCQEntry, pRdReq->bsDecFifoIndex);
    }

    /*  NFCQ DW8-15: flash address  */
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);

    /*  DSG DW0: transfer length and chain config  */
    pDSG->bsXferByteLen = (pRdReq->bsSecLen) << SEC_SZ_BITS;
    pDSG->bsLast = TRUE;
    /*  DSG DW1: dram address  */
    pDSG->bsDramAddr =  HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, pRdReq->bsSecStart, ucBufUnit);
    //DBG_Printf("PU%d page%d bsDramAddr= 0x%x\n",ucPU,pFlashAddr->usPage, pDSG->bsDramAddr<<1);
    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr,  ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSinglePlaneWrite
Description:
    write DRAM data to flash by single-plane
Input Param:n
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U16 usBuffID: buffer id of DRAM
    NFC_RED *pRed: pointer to readundant data
    BOOL bIsRedOntf: 1-OTFB, 0-DRAM
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
    20151106    abby    modify to meet VT3533, dynamic update RED to OTFB/DRAM
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlaneWrite(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq)
{
    U8  ucPU, ucLun, ucPln, ucReqType;
    U8  ucRedPlnLen;
    U16 usCurDsgId, usPage;
    U32 ulPgSz;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;
    NFC_RED *pTargetRed;
    NFC_SCR_CONF eScrType;
    BOOL bIsRedOntf = pWrReq->bsRedOntf;
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    U8  ucDsIndex = pWrReq->bsDsIndex;
    U8  ucBufUnit = ((pDsReg->atDSEntry[ucDsIndex].bsCwNum % 16) ? CW_INFO_SZ_BITS : LOGIC_PG_SZ_BITS);
    U32 ulPgUnit  = (pDsReg->atDSEntry[ucDsIndex].bsCwNum % 16) ? (LOGIC_PG_SZ - CW_INFO_SZ) : LOGIC_PG_SZ;
    
    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPln = pFlashAddr->bsPln;
    usPage = pFlashAddr->usPage;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (FALSE == HAL_NfcIsPairPageAccessable(usPage))
    {
        return NFC_STATUS_FAIL;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    
    /* adapter for different flash types */
    if (pWrReq->bsTlcMode)//MLC
    {
        if (pWrReq->bsSinglePgProg)//single page programming mode
        {
            ulPgSz      = ulPgUnit;
            eScrType    = NORMAL_SCR;
            ucRedPlnLen = 1;
            ucReqType   = FLASH_PRCQ_MLC_PRG_SING_PG_1PLN;  
        }
        else
        {
            ulPgSz      = ulPgUnit * PGADDR_PER_PRG;
            eScrType    = MLC_RW_TWO_PG;
            ucRedPlnLen = INTRPG_PER_PGADDR * PGADDR_PER_PRG;
            ucReqType   = NF_PRCQ_MLC_PRG;  
            
            FLASH_ADDR tFlashAddr = *pFlashAddr;
            tFlashAddr.usPage++;
            pNFCQEntry->atRowAddr[1].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
        }
    }
    else
    {
        ulPgSz      = ulPgUnit;
        eScrType    = NORMAL_SCR;
        ucRedPlnLen = 1;
        ucReqType   = NF_PRCQ_PRG;   
    }
    //DBG_Printf("1PLN WR page%d ScrType%d reqType%d RedPlnLen%d pageSize%dKB\n",pFlashAddr->usPage, eScrType, ucReqType, ucRedPlnLen, ulPgSz/1024);

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = ucDsIndex;
    pNFCQEntry->bsEMEn    = pWrReq->bsEmEn;

    /*  DSG configure   */
    pNFCQEntry->bsDsgEn = TRUE;
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pWrReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pWrReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    /*    Note: for write cmd, the unit of DmaTotalLength is 1KB */
    pNFCQEntry->bsDmaTotalLength = ulPgSz >> CW_INFO_SZ_BITS;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pWrReq->pNfcRed)
    {
        pNFCQEntry->bsRedOntf = bIsRedOntf;
        pNFCQEntry->bsRedEn = TRUE;
        pTargetRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun,HAL_NfcGetWP(ucPU, ucLun), ucPln, bIsRedOntf);
        COM_MemCpy((U32 *)pTargetRed, (U32 *)(pWrReq->pNfcRed), RED_SZ_DW * ucRedPlnLen);
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU, ucLun,HAL_NfcGetWP(ucPU, ucLun), ucPln, bIsRedOntf);
    }
    HAL_ConfigScramble(pNFCQEntry, usPage, SCR_SINGLE_PLN, 0, eScrType);

    /*  NFCQ DW8-15: flash address  */
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG, sizeof(NORMAL_DSG_ENTRY)>>2);
    /*  DSG DW0: transfer length and chain config  */
    pDSG->bsXferByteLen = ulPgSz;
    pDSG->bsLast = TRUE;

    /*  DSG DW1: dram address  */

    pDSG->bsDramAddr = HAL_NfcGetDmaAddr(pWrReq->bsWrBuffId, 0, ucBufUnit);

    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSinglePlnCCLRead
Description:
    single plane change column read
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    NFC_READ_REQ_DES *pRdReq;
Output Param:
    none
Return Value:
    BOOL: TRUE:  build a single plane read cmd success
          FALSE: build a single plane read cmd fail
Usage:
History:
    20160308    abby    create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlnCCLRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    U8  ucPU, ucLun, ucPln;
    U8  ucPageType = 0;
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;
    NFC_SCR_CONF eScrType = NORMAL_SCR;

    ucPU   = pFlashAddr->ucPU;
    ucLun  = pFlashAddr->ucLun;
    ucPln  = pFlashAddr->bsPln;
    
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }
    
    ucPageType = 0;
    ucPln = pFlashAddr->bsPln;
    eScrType = NORMAL_SCR;

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    pNFCQEntry->bsEMEn    = pRdReq->bsEmEn;

    /*  DSG configure   */
    while(FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsDsgEn = TRUE;
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW1: sector enable and length   */
    pNFCQEntry->aSecAddr[0].bsSecStart = pRdReq->bsSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = pRdReq->bsSecLen;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pRdReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pRdReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    pNFCQEntry->bsDmaTotalLength = pRdReq->bsSecLen;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_CCLR);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pRdReq->ppNfcRed)
    {
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedOntf = pRdReq->bsRedOntf;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,pRdReq->bsRedOntf);
        *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,(pRdReq->bsRedOntf));
    }
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_SINGLE_PLN, ucPageType, eScrType);

    /*  NFCQ DW5: LBA for read, fill by FW, the same with corresponding LBA in write red   */
    if (pRdReq->bsLbaChkEn || pRdReq->bsEmEn)
    {
        pNFCQEntry->bsRdLba = pRdReq->bsLba;
    }

    /* DEC FIFO status report config */
    if (FALSE != pRdReq->bsDecFifoEn)
    {
        HAL_DecFifoTrigNfc(pNFCQEntry, pRdReq->bsDecFifoIndex);
    }

    /*  NFCQ DW8-15: flash address  */
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);
    /*  DSG DW0: transfer length and chain config  */
    pDSG->bsXferByteLen = (pRdReq->bsSecLen) << SEC_SZ_BITS ;
    pDSG->bsLast = TRUE;
    /*  DSG DW1: dram address  */
    pDSG->bsDramAddr =  HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, pRdReq->bsSecStart, LOGIC_PG_SZ_BITS);
    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_CCLR, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

GLOBAL BOOL MCU12_DRAM_TEXT HAL_NfcDetectEmptyPage(NFC_RED *p_Red, U8 ErrCode)
{
    BOOL bEmptyPage = FALSE;
    U32 ulIndex, ulTotalDWNum = 4;
    U32 ulBit1Cnt = 0;
    U32 ulBit1Pos = 0;
    U32 ulData;

    if ((NULL != p_Red) && ((NF_ERR_TYPE_UECC == ErrCode) || (NF_ERR_TYPE_DCRC == ErrCode)))
    {
        for (ulIndex = 0; ulIndex < ulTotalDWNum; ulIndex++)
        {
            ulData = p_Red->aContent[ulIndex];
            while (ulData != 0)
            {
                if ((ulData & 1) != 0)
                {
                    ulBit1Cnt++;
                }
                ulData >>= 1;
            }
        }
        if (ulTotalDWNum * 4 * 8 - ulBit1Cnt < 4)
        {
            //DBG_Printf("MCU#%d ErrCode %d Detect to Empty Page.\n", HAL_GetMcuId(), ErrCode);
            bEmptyPage = TRUE;
        }
    }

    return bEmptyPage;
}

/*==============================================================================
Func Name  : HAL_NfcSetRedInDramBase
Input      : U32 ulRedBaseAddr
Output     : NONE
Return Val : GLOBAL
Discription: if we need to transfer red data with nfc by dram space, we should
             call this interface to init the Red base address before executing
             the program or read flash command.
Usage      :
History    :
    1. 2016.3.18 JasonGuo create function
==============================================================================*/
void MCU12_DRAM_TEXT HAL_NfcSetRedInDramBase(U32 ulRedBaseAddr)
{
    volatile NF_RED_DRAM_BASE_REG *pRedBaseAddr;

    pRedBaseAddr = (volatile NF_RED_DRAM_BASE_REG *)&rNfcRedDramBase;

    *(U32*)pRedBaseAddr = 0;
    pRedBaseAddr->bsRedDramBaseAddr = (ulRedBaseAddr-DRAM_START_ADDRESS)>>3;

    return;
}

/*==============================================================================
Func Name  : HAL_NfcSetOtfbAdsBase
Input      : U32 ulBaseAddr0, U32 ulBaseAddr1, U32 ulBaseAddr2
Output     : NONE
Return Val : GLOBAL
Discription: Init the Otfb ADS Base address for Data transfer by OTFB path.
Usage      : we can select the Base address by OTFB_BASE_SEL in NFCQ_ENTRY.
History    :
1. 2016.3.18 JasonGuo create function
==============================================================================*/
void MCU12_DRAM_TEXT HAL_NfcSetOtfbAdsBase(U32 ulBaseAddr0, U32 ulBaseAddr1, U32 ulBaseAddr2)
{
    U32 ulBaseOtfbAdr0 = ulBaseAddr0>>20;
    U32 ulBaseOtfbAdr1 = ulBaseAddr1>>20;
    U32 ulBaseOtfbAdr2 = ulBaseAddr2>>20;

    REG_SET_VALUE(rNfcMisc, 12, ulBaseOtfbAdr0, 16);
    REG_SET_VALUE(rNfcOtfbAdsBase, 12, ulBaseOtfbAdr1, 0);
    REG_SET_VALUE(rNfcOtfbAdsBase, 12, ulBaseOtfbAdr2, 16);

    return;
}

/*==============================================================================
Func Name  : HAL_NfcSetSsuInOtfbBase
Input      : U32 ulSsuBaseAddr
Output     : NONE
Return Val : GLOBAL
Discription: if we need the nfc update the otfb-space status autoly, we should
             call this interface to set the ssu base address in otfb before executing
             flash command.
Usage      :
History    :
1. 2016.3.18 JasonGuo create function
==============================================================================*/
void MCU12_DRAM_TEXT HAL_NfcSetSsuInOtfbBase(U32 ulSsuBaseAddr)
{
    volatile NF_SSU_BASEADDR_REG *pSsuBaseAddr;
    pSsuBaseAddr = (volatile NF_SSU_BASEADDR_REG *)&rNfcSsuBaseAddr;

    pSsuBaseAddr->bsSsuOtfbBase = (ulSsuBaseAddr - OTFB_START_ADDRESS) >> 10;

    return;
}

/*==============================================================================
Func Name  : HAL_NfcSetSsuInDramBase
Input      : U32 ulSsuBaseAddr
Output     : NONE
Return Val : GLOBAL
Discription: if we need the nfc update the dram-space status autoly, we should
             call this interface to set the ssu base address in otfb before executing
             flash command.
Usage      :
History    :
1. 2016.3.18 JasonGuo create function
==============================================================================*/
void MCU12_DRAM_TEXT HAL_NfcSetSsuInDramBase(U32 ulSsuBaseAddr)
{
    volatile NF_SSU_BASEADDR_REG *pSsuBaseAddr;
    pSsuBaseAddr = (volatile NF_SSU_BASEADDR_REG *)&rNfcSsuBaseAddr;

    pSsuBaseAddr->bsSsuDramBase = (ulSsuBaseAddr - DRAM_START_ADDRESS) >> 10;

    return;
}

/*==============================================================================
Func Name  : HAL_NfcBypassScrb
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.17 JasonGuo create function
==============================================================================*/
BOOL HAL_NfcBypassScrb(void)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *)&rNfcPgCfg;

    return (0 == pNfcPgCfg->bsScrBps) ? FALSE : TRUE;
}

/*==============================================================================
Func Name  : HAL_NfcIsChkSeedSel
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.17 JasonGuo create function
==============================================================================*/
BOOL HAL_NfcIsChkSeedSel(void)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *)&rNfcPgCfg;

    return (1 == pNfcPgCfg->bsRTSB3dTlc) ? TRUE : FALSE;
}

/* end of file HAL_FlashDriverBasic.c */

