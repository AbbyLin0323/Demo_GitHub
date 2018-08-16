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
*******************************************************************************/

#include "Disk_Config.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_ParamTable.h"
#include "win_bootloader.h"
#include "model_common.h"
#include "model_config.h"
#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#endif

/*------------------------------------------------------------------------------
Name: SIM_WinGenCEBitMap
Description: 
    Simulate the ReadID of BootLoader to get the real ce bitmap in Win Env.
Input Param:
    U32 ulMcuId: the current subsystem id.
Output Param:
    none
Return Value:
    U32 ulBitMap: the current subsystem ce bitmap.
Usage:
    In FW. call this funtion to simulate the ReadID of BootLoader to get the 
    real ce bitmap in Win Env.
History:
    20141027    Jason   modify.
------------------------------------------------------------------------------*/
LOCAL U32 SIM_WinGenCEBitMap(U32 ulMcuId)
{
#ifdef SINGLE_SUBSYSTEM

    U32 ulBitMap;

    ulBitMap = MULTI_VALUE_1(CE_SUM);

    return ulBitMap;
#else

    U32 ulBitMap, ulCE;

    ulBitMap = 0;
    for (ulCE = 0; ulCE < CE_SUM; ulCE++)
    {   
        if ((MCU1_ID == ulMcuId) && (0 == ulCE%2))
        {
            ulBitMap |= 1<<ulCE;
        }
        
        if ((MCU2_ID == ulMcuId) && (1 == ulCE%2))
        {
            ulBitMap |= 1<<ulCE;
        }        
    }
    
    return ulBitMap;
#endif    
}

/*------------------------------------------------------------------------------
Name: SIM_InitBootParamTable
Description: 
    To simulate the behavior of BootLoader in Win Env.
Input Param:
    BOOL bLLF: Need LLF Or Not?
Output Param:
    none
Return Value:
    none
Usage:
    In FW. call this funtion to simulate the behavior of BootLoader in Win Env
History:
    20141027    Jason   modify.
------------------------------------------------------------------------------*/
LOCAL void SIM_InitBootParamTable(BOOL bLLF)
{
    U8 ucFuncType;
    BOOTLOADER_FILE *pBootFileAddr = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
    PTABLE *pPTable = (PTABLE *)&(pBootFileAddr->tSysParameterTable);
    FTABLE *pFTable = (FTABLE *)&(pBootFileAddr->tSysFunctionTable);

    pBootFileAddr->ulBLSignatureDW0 = SIGNATURE_DW0;
    pBootFileAddr->ulBLSignatureDW1 = SIGNATURE_DW1;

    // set boot method
    if (TRUE == bLLF)
    {
        pPTable->sBootStaticFlag.bsBootMethodSel = BOOT_METHOD_LLF;
    }
    else
    {
        pPTable->sBootStaticFlag.bsBootMethodSel = BOOT_METHOD_NORMAL;
    }

    // set boot flag
    pPTable->sBootSelFlag.bsEnableMCU0 = 1;
    pPTable->sBootSelFlag.bsEnableMCU1 = 1;

#ifdef SINGLE_SUBSYSTEM    
    pPTable->sBootSelFlag.bsEnableMCU2 = 0;
    pPTable->ulSubSysNum = 1;
    pPTable->ulSubSysCEMap[0] = SIM_WinGenCEBitMap(MCU1_ID);
#else
    pPTable->sBootSelFlag.bsEnableMCU2 = 1;
    pPTable->ulSubSysNum = 2;
    pPTable->ulSubSysCEMap[0] = SIM_WinGenCEBitMap(MCU1_ID);
    pPTable->ulSubSysCEMap[1] = SIM_WinGenCEBitMap(MCU2_ID);
#endif

    pPTable->ulSubSysCeNum = BL_SUBSYSTEM_PU_NUM;
    pPTable->ulSubSysMaxLBACnt = MAX_LBA_IN_DISK;

    pPTable->sHwInitFlag.ulHWInitFlag = INVALID_8F;

    /* FW feature settings in P-Table */

    /* SATA L0 - 0: HIPM; 1: DIPM; 2: HPA 3: Security; 4: SSP; */
    pPTable->tSATAL0Feature.ulFeatureBitMap = 0x1F; 

    /* AHCI L0 - 0: HIPM; 1: DIPM; 2: HPA 3: Security; 4: SSP; */
    pPTable->tAHCIL0Feature.ulFeatureBitMap = 0x1F;

    /* NVMe L0 - bit0-1: MSI/MSIX select; bit2: LBA size 512B/4KB select; */
    pPTable->tNVMeL0Feature.ulFeatureBitMap = 0x1;

    /* L1 - 0: LBA Hashing Enable */
    pPTable->tL1Feature.ulFeatureBitMap = 0x0;

    /* L2: init parameter table */
    pPTable->tL2Feature.aFeatureDW[0] = 0xC64;   //GC_THRESHOLD_PAGE_CNT + 100
    pPTable->tL2Feature.aFeatureDW[1] = 5;      //TOO_COLD_BLK
    pPTable->tL2Feature.aFeatureDW[2] = 20;     //g_WLEraseCntThs
    pPTable->tL2Feature.aFeatureDW[3] = 512;    //DWA_EN_SUSTAIN_THS, 8 CE total write 128M byte to start DWA.

    /* HAL - bit[0] = 1: HW Debug Trace Enable */
    pPTable->tHALFeature.ulFeatureBitMap = 0x0;

    for (ucFuncType = 0; ucFuncType < INIT_FUNC_CNT; ucFuncType++)
    {
        pFTable->aInitFuncEntry[ucFuncType].ulEntryAddr = INVALID_8F;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: SIM_WinGetCEBitMap
Description: 
    get ce bit map.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In Win only. call this funtion to get ce bit map of current mcu.
History:
    20141024    Jason   create.
------------------------------------------------------------------------------*/
LOCAL U32 SIM_WinGetCEBitMap(U32 ulMcuId)
{
    U32 ulBitMap;
    BOOTLOADER_FILE *pBootFileAddr = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;

    if ((MCU1_ID != ulMcuId) && (MCU2_ID != ulMcuId))
    {
        DBG_Printf("SIM_WinGetCEBitMap %d Error.\n",ulMcuId);
        DBG_Getch();
    }
    
    ulBitMap = pBootFileAddr->tSysParameterTable.ulSubSysCEMap[ulMcuId - MCU1_ID];

    return ulBitMap;
}

/*------------------------------------------------------------------------------
Name: SIM_WinSetLogicPUReg
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
    In Win only. call this funtion to Initialize LogicPU register for FW.
    Sync with HAL_NfcSetLogicPUReg() which is used in cosim/xtmp/fpga/asic evn.
History:
    20141024    Jason   create.
------------------------------------------------------------------------------*/
LOCAL void SIM_WinSetLogicPUReg(U32 ulSubsys0CEBitMap, U32 ulSubsys1CEBitMap)
{   
    U8 ucCE;   
    U8 ucLogicPU = 0;

    NFC_LOGIC_PU *pNfcLogicPU = (NFC_LOGIC_PU *)LOGIC_PU_REG_BASE;

    if (0 != (ulSubsys0CEBitMap & ulSubsys1CEBitMap))
    {
        DBG_Getch();
    }
    
    /* set LogicPU register into a no map sts */
    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable = FALSE;   
        pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = 0;                                                              
    }

    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        if (0 != (ulSubsys0CEBitMap & (1 << ucCE)))
        {
            pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable = TRUE;   
            pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = ucLogicPU;       
            ucLogicPU++;
        }       
    }

    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        if (0 != (ulSubsys1CEBitMap & (1 << ucCE)))
        {
            pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable = TRUE;   
            pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = ucLogicPU;       
            ucLogicPU++;
        }       
    }

    // init the left hw reg
    for (ucCE = 0; ucCE < CE_MAX; ucCE++)
    {
        if ((0 == pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsPuEnable)
         && (0 == pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU))
        {
            pNfcLogicPU->aNfcLogicPUReg[NFC_CH_NUM(ucCE)][PU_IN_CH(ucCE)].bsLogicPU = ucLogicPU++;
        }
    }

    return;
}


/*------------------------------------------------------------------------------
Name: SIM_WinBootLoader
Description: 
    To simulate the behavior of BootLoader in Win Env.
Input Param:
    BOOL bLLF: Need LLF Or Not?
Output Param:
    none
Return Value:
    none
Usage:
    In FW. call this funtion to simulate the behavior of BootLoader in Win Env
History:
    20141027    Jason   modify.
------------------------------------------------------------------------------*/
void SIM_WinBootLoader(BOOL bLLF)
{
    SIM_InitBootParamTable(bLLF);
    
    SIM_WinSetLogicPUReg(SIM_WinGetCEBitMap(MCU1_ID),SIM_WinGetCEBitMap(MCU2_ID));

    return;
}


U32 SIM_CaculateLBA(U8 uPUNum)
{
    U32 ulSizeByG;
    ulSizeByG = ((BLK_PER_CE*LPN_PER_BLOCK*(LPN_SIZE/1024))/(1024*1024))*uPUNum ;
    return (97696368 + (1953504 * (ulSizeByG - 50)));
}



