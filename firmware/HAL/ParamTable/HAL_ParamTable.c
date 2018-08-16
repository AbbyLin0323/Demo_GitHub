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

GLOBAL U32 g_ulPuNum;    // Added for suit Dynamic PU_NUM func
LOCAL BOOL l_bPMUOldSts;
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

U8 HAL_GetSubSysCnt()
{
    PTABLE * pPtable;
    pPtable = HAL_GetPTableAddr();
    return pPtable->ulSubSysNum;
}

U32 HAL_GetPrintDRAMStartAddr()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->tPrintToDRAMFeature.uPrinttoDRAMStartAddr;
}

U32 HAL_GetPrintDRAMSize()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->tPrintToDRAMFeature.uPrinttoDRAMSize;
}

BOOL HAL_IsBootLoaderNeedDelay()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->tPrintToDRAMFeature.bsBootLoaderDelayEnable;
}

BOOL HAL_GetPrinttoDRAMEnFlag()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->sBootStaticFlag.bsPrinttoDRAMEn;
}

BOOL HAL_GetWarmBootFlag()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->sBootStaticFlag.bsWarmBoot;
}

BOOL HAL_GetTraceLogEnFlag()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->sBootStaticFlag.bsTraceLogEn;
}

BOOL HAL_IsRamDiskMode()
{
    PTABLE * pPtable;
    pPtable = HAL_GetPTableAddr();

    if (pPtable->sBootStaticFlag.bsRamDiskMode == TRUE)
    {
        return TRUE;
    }

    return FALSE;
}

U32 HAL_GetTraceLogMCU0StartAddr()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->tTraceLogDRAMFeature.uTraceLogStartAddr_MCU0;
}

U32 HAL_GetTraceLogMCU1StartAddr()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->tTraceLogDRAMFeature.uTraceLogStartAddr_MCU1;
}

U32 HAL_GetTraceLogMCU2StartAddr()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->tTraceLogDRAMFeature.uTraceLogStartAddr_MCU2;
}

U32 HAL_GetTraceLogDRAMSize()
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->tTraceLogDRAMFeature.uTraceLogDRAMSize;
}

/*----------------------------------------------------------------------------
Name: HAL_GetDataPacketMethod
Description:
    get bsDataPacketMethod from ptable
Input Param:
Output Param:
Return Value:  bsDataPacketMethod bit, 0: using SATA packet, 1: using UART packet
Usage:

History:
    20161117 Eason Chien create
------------------------------------------------------------------------------*/

U8 HAL_GetDataPacketMethod(void)
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->sBootStaticFlag.bsDataPacketMethod;
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
    
    if (ucFuncType >= FTABLE_FUNC_TOTAL)
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
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    if ((MCU1_ID != ulMcuId) && (MCU2_ID != ulMcuId))
    {
        DBG_Getch();
    }

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;
        
    return pPTable->ulSubSysCEMap[ulMcuId-MCU1_ID];
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
BOOL HAL_IsBaudRate12MEnable()
{
    PTABLE * pPTable;
    BOOTLOADER_FILE * pBootLoaderFile;

    pBootLoaderFile = HalGetBootLoaderFileBase();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    return pPTable->sBootStaticFlag.bsUart12MEnable;
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

    pBootLoaderFile = (BOOTLOADER_FILE*)(DRAM_BOOTLOADER_PART0_BASE);
    pPTable = &pBootLoaderFile->tSysParameterTable;
    tBootFlag = pPTable->sBootStaticFlag;
    tHwIntFlag = pPTable->sHwInitFlag;

    tBootFlag.bsBootMethodSel = BOOT_METHOD_NORMAL;
    tHwIntFlag.ulHWInitFlag = 0;

    if (TRUE == tBootFlag.bsUartMpMode)
    {
        /*close UartMpMode*/
        tBootFlag.bsUartMpMode = FALSE;
    }

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

/*----------------------------------------------------------------------------
Name: HAL_GetBootMethod
Description:
    get boot method from ptable 
Input Param:
Output Param: 
Return Value:  Boot method
Usage:
    
History:
    20150821 Victor Zhang create
------------------------------------------------------------------------------*/

U8 HAL_GetBootMethod(void)
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->sBootStaticFlag.bsBootMethodSel;
}


/*----------------------------------------------------------------------------
Name: HAL_GetInheritEraseCntEnFlag
Description:
get InheritEraseCntEn flag from ptable
Input Param:
Output Param:
Return Value: TRUE --L2 inherit EraseCnt
                 FALSE --L2 not inherit EraseCnt
Usage:

History:
20151229 Tobey create
------------------------------------------------------------------------------*/
BOOL HAL_GetInheritEraseCntEnFlag(void)
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return pPTable->sBootStaticFlag.bsInheritEraseCntEn;
}

BOOL HAL_GetPbnBindingTableEnFlag(void)
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return (pPTable->sBootStaticFlag.bsPbnBindingTableEn != 0) ? TRUE : FALSE;
}

/*----------------------------------------------------------------------------
Name: HAL_GetTemperatureSensorI2C
Description:
get Temperature Sensor relative info from ptable
Input Param:
Output Param:
Return Value: 
Usage:
------------------------------------------------------------------------------*/
TEMPERATURE_SENSOR_I2C * HAL_GetTemperatureSensorI2C(void)
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return &pPTable->tTemperatureSensorI2C;
}

TEMPERATURE_SENSOR_TSC * HAL_GetTSCSmartValue(void)
{
    PTABLE *pPTable;
    pPTable = HAL_GetPTableAddr();
    return &pPTable->tTemperatureSensorTSC;
}

