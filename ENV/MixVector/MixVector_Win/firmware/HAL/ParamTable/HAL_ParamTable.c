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
Filename    : HAL_ParamTable.c
Version     : Ver 1.0
Author      : Tobey
Date        : 2014.09.29
Description : PTable and FTable relative driver for FW use.
Others      : 
Modify      :
20140929    Tobey     Create file
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_ParamTable.h"
#include "HAL_xtensa.h"

GLOBAL MCU12_VAR_ATTR U32 g_ulPuNum;    // Added for suit Dynamic PU_NUM func
/*----------------------------------------------------------------------------
Name: HalGetBootLoaderFileBase
Description:
    Get BootLoaderFile base addr
Input Param:
    none
Output Param:
    none
Return Value:
    BOOTLOADER_FILE *:pointer of base addr
Usage:
    invoke this interface to Get BootLoaderFile base addr
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
LOCAL BOOTLOADER_FILE * HalGetBootLoaderFileBase(void)
{
    return (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
}

/*----------------------------------------------------------------------------
Name: HAL_GetPTableAddr
Description:
    Get P-table base addr
Input Param:
    none
Output Param:
    none
Return Value:
    PTABLE *:pointer to P-Table
Usage:
    invoke this interface to Get P-Table addr
History:
    20150318    fisrt create
------------------------------------------------------------------------------*/
PTABLE * HAL_GetPTableAddr(void)
{
    BOOTLOADER_FILE *pBootloaderFile = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
    
    return &(pBootloaderFile->tSysParameterTable);
}

/*----------------------------------------------------------------------------
Name: HAL_IsBootLoaderFileExist
Description: 
    Check BootLoaderFile exist on OTFB or not.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE: Exist; FALSE:not Exist
Usage:
    invoke this interface to check BootLoaderFile exist on OTFB or not.
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
BOOL HAL_IsBootLoaderFileExist(void)
{
    BOOL bExistFlag = FALSE;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    if ((SIGNATURE_DW0 == pBootLoaderFile->ulBLSignatureDW0)
    && (SIGNATURE_DW1 == pBootLoaderFile->ulBLSignatureDW1))
    {
        bExistFlag = TRUE;
    }
    return bExistFlag;
}

/*----------------------------------------------------------------------------
Name: HAL_InitGlobalPUNum
Description: 
    initialize g_ulPuNum for FW use. get it form PTable.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW or BootLoader invoke this interface to initialize g_ulPuNum at initial stage.
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
void HAL_InitGlobalPUNum(void)
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;
    g_ulPuNum = pPTable->ulSubSysCeNum;

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SetPTableDoneBit
Description:
    set HW Init done bit.
Input Param:
    U8 ucFuncType:HW Init type
Output Param:
    none
Return Value:
    void
Usage:
    invoke this interface to set HW Init done bit.
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
void HAL_SetPTableDoneBit(U8 ucFuncType)
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    if (ucFuncType >= INIT_FUNC_CNT)
    {
        DBG_Getch();
    }
    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;
    pPTable->sHwInitFlag.ulHWInitFlag |= 1<<ucFuncType;

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_ClearPTableDoneBit
Description: 
    clear HW Init done bit.
Input Param:
    U8 ucFuncType:HW Init type
Output Param:
    none
Return Value:
    void
Usage:
    invoke this interface to clear HW Init done bit.
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
void HAL_ClearPTableDoneBit(U8 ucFuncType)
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;
    
    if (ucFuncType >= INIT_FUNC_CNT)
    {
        DBG_Getch();
    }
    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;
    pPTable->sHwInitFlag.ulHWInitFlag &= ~(1<<ucFuncType);
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_IsPTableDone
Description:
    check one HW Init work all done or not
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE: done; FALSE:not done;
Usage:
    invoke this interface to check one HW Init work is done or not
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
BOOL HAL_IsPTableDone(U8 ucFuncType)
{
    BOOL bDone;
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    if (ucFuncType >= INIT_FUNC_CNT)
    {
        DBG_Getch();
    }

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    if (0 == ((1<<ucFuncType) & pPTable->sHwInitFlag.ulHWInitFlag))
    {
        bDone = FALSE;
    }
    else
    {
        bDone = TRUE;
    }

    return bDone;
}

/*----------------------------------------------------------------------------
Name: HAL_IsPTableAllDone
Description: 
    check all HW Init work all done or not
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:all done; FALSE:not all done;
Usage:
    invoke this interface to check all HW Init work all done or not
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
BOOL HAL_IsPTableAllDone(void)
{
    U8 ucDoneBit;
    BOOL bAllDone;
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    ucDoneBit = 0;
    while (ucDoneBit < INIT_FUNC_CNT)
    {
        if (0 == ((1<<ucDoneBit) & pPTable->sHwInitFlag.ulHWInitFlag))
        {
            break;
        }
        ucDoneBit++;
    }
    
    if (ucDoneBit < INIT_FUNC_CNT)
    {
        bAllDone = FALSE;
    }
    else
    {
        bAllDone = TRUE;
    }
    
    return bAllDone;
}

/*----------------------------------------------------------------------------
Name: HAL_IsFTableValid
Description:
    check FTable valid or not throught check one FuncEntry are initialized or not.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:valid; FALSE:invalid;
Usage:
    invoke this interface to check FTable one FuncEntry valid or not
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
BOOL HAL_IsFTableValid(U8 ucFuncType)
{
    BOOL bValid;
    FTABLE * pFTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    if (ucFuncType >= INIT_FUNC_CNT)
    {
        DBG_Getch();
    }

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pFTable = &pBootLoaderFile->tSysFunctionTable;

    if ((U32)NULL == pFTable->aInitFuncEntry[ucFuncType].ulEntryAddr)
    {
        bValid = FALSE;
    }
    else
    {
        bValid = TRUE;
    }

    return bValid;
}

/*----------------------------------------------------------------------------
Name: HAL_IsFTableAllValid
Description:
    check FTable valid or not throught check all FuncEntry ulEntryAddr
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:valid; FALSE:invalid;
Usage:
    invoke this interface to check FTable all FuncEntry valid or not
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
BOOL HAL_IsFTableAllValid(void)
{
    U8 ucFuncBit;
    BOOL bValid;
    FTABLE * pFTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pFTable = &pBootLoaderFile->tSysFunctionTable;

    ucFuncBit = 0;
    while (ucFuncBit < INIT_FUNC_CNT)
    {
        if ((U32)NULL == pFTable->aInitFuncEntry[ucFuncBit].ulEntryAddr)
        {
            break;
        }
        ucFuncBit++;
    }

    if (ucFuncBit < INIT_FUNC_CNT)
    {
        bValid = FALSE;
    }
    else
    {
        bValid = TRUE;
    }

    return bValid;
}

/*----------------------------------------------------------------------------
Name: HAL_RegisterFTableFunc
Description: 
    register a FTable FuncEntry.
Input Param:
    U8 ucFuncType:register func type
    U32 ulFuncAddr:the real func pointer addr
Output Param:
    none
Return Value:
    void
Usage:
    invoke this interface to register a FTable FuncEntry.
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
void HAL_RegisterFTableFunc(U8 ucFuncType, U32 ulFuncAddr)
{
    FTABLE * pFTable;
    BOOTLOADER_FILE * pBootLoaderFile;
    
    if (ucFuncType >= INIT_FUNC_CNT)
    {
        DBG_Getch();
    }

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pFTable = &pBootLoaderFile->tSysFunctionTable;
    pFTable->aInitFuncEntry[ucFuncType].ulEntryAddr = ulFuncAddr;
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_InvokeFTableFunc
Description: 
    invoke a func registed in FTable FuncEntry;
Input Param:
    U8 ucFuncType:register func type
Output Param:
    none
Return Value:
    void
Usage:
    invoke this interface to invoke a func registed in FTable FuncEntry;
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
void HAL_InvokeFTableFunc(U8 ucFuncType)
{
    FTABLE * pFTable;
    FunPointer pFuncPointer;
    BOOTLOADER_FILE * pBootLoaderFile;
    
    if (ucFuncType >= INIT_FUNC_CNT)
    {
        DBG_Getch();
    }

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pFTable = &pBootLoaderFile->tSysFunctionTable;
    pFuncPointer = (FunPointer)pFTable->aInitFuncEntry[ucFuncType].ulEntryAddr;
    pFuncPointer();
    
    HAL_SetPTableDoneBit(ucFuncType);
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GetFTableFuncAddr
Description: 
    get FTable func pointer addr by FuncType.
Input Param:
    U8 ucFuncType:register func type
Output Param:
    none
Return Value:
    U32:FTable func pointer addr
Usage:
    FW invoke this interface to get FTable func pointer addr;
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
U32 HAL_GetFTableFuncAddr(U8 ucFuncType)
{
    FTABLE * pFTable;
    BOOTLOADER_FILE * pBootLoaderFile;
    
    if (ucFuncType >= INIT_FUNC_CNT)
    {
        DBG_Getch();
    }

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pFTable = &pBootLoaderFile->tSysFunctionTable;

    return pFTable->aInitFuncEntry[ucFuncType].ulEntryAddr;
}

/*----------------------------------------------------------------------------
Name: HAL_GetSubSystemCEMap
Description:
    FW get SubSystemCEMap.
Input Param:
    U32 ulMcuId:Mcu id
Output Param:
    none
Return Value:
    U32:SubSystemCEMap
Usage:
    FW this interface to get SubSystemCEMap.
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
U32 HAL_GetSubSystemCEMap(U32 ulMcuId)
{
    U8 ucSubSystemId;
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;
    ucSubSystemId = ulMcuId - MCU1_ID;

    if (ucSubSystemId >= pPTable->ulSubSysNum)
    {
        DBG_Getch();
    }
    
    return pPTable->ulSubSysCEMap[ucSubSystemId];
}

void HAL_SetDebugMode(void)
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;
    pPTable->sBootStaticFlag.bsDebugMode = TRUE;

    return;
}

BOOL HAL_ISDebugMode(void)
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    return pPTable->sBootStaticFlag.bsDebugMode;
}
/*----------------------------------------------------------------------------
Name: HAL_IsMCUEnable
Description:
    Check one MCU should be run or not.
Input Param:
    U32 ulMcuId:Mcu id
Output Param:
    none
Return Value:
    BOOL:TRUE:should run; FALSE: shouldn't run;
Usage:
    FW invoke this interface to Check one MCU should be run or not.
History:
    201409029    fisrt create
------------------------------------------------------------------------------*/
BOOL HAL_IsMCUEnable(U32 ulMcuId)
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;
    BOOL bBootUp = FALSE;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    if (0 != (pPTable->sBootSelFlag.ulEnableMCUFlag & (1 << (ulMcuId - 1))))
    {
        bBootUp = TRUE;
    }

    return bBootUp;
}

/*----------------------------------------------------------------------------
Name: HAL_IsMCUEnable
Description:
    prepare boot method and HW init flags for normal boot.
Input Param:
    U32 ulMcuId:Mcu id
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke this to prepare boot method and HW init flags for normal boot.
    invoke by FW when save BootLoader to flash by the first time.
History:
    20141231    fisrt create
------------------------------------------------------------------------------*/
void HAL_PrepareFlagsForNormalBoot(void)
{
    PTABLE * pPTable;
    HW_INIT_FLAG tHwIntFlag;
    BOOT_STATIC_FLAG tBootFlag;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;
    tBootFlag = pPTable->sBootStaticFlag;
    tHwIntFlag = pPTable->sHwInitFlag;

    tBootFlag.bsBootMethodSel = BOOT_METHOD_NORMAL;
    tHwIntFlag.ulHWInitFlag = 0;

    pPTable->sBootStaticFlag = tBootFlag;
    pPTable->sHwInitFlag = tHwIntFlag;
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GetFlashChipId
Description:
    get flash chip id,used to distinguish L95/L85/TSB
Input Param:
    U32 ucIndex
Output Param:
    U32
Return Value:
    flashid
Usage:
    FlashChipId report to host tool by VarTable
History:
    20150312  NinaYang  fisrt create
------------------------------------------------------------------------------*/

U32 HAL_GetFlashChipId(U8 ucIndex)
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    return pPTable->aFlashChipId[ucIndex];
}

