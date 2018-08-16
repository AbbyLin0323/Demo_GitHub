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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_Dmae.h"
#include "HAL_PM.h"
#include "HAL_Head.h"
#include "system_statistic.h"
#include "sim_flash_common.h"
#include "sim_flash_shedule.h"
#include "sim_search_engine.h"
#include "sim_DMAE.h"
#include "sim_HSG.h"
#include "sim_NormalDSG.h"
#include "HostModel.h"
#include "memory_access.h"
#include "xtmp_config.h"
#include "xtmp_options.h"
#include "xtmp_common.h"
#ifdef HOST_SATA
#include "simsatadev.h"
#endif

extern BOOL nfcRegWrite(U32 regaddr, U32 regvalue, U32 len);
extern BOOL sgeRegWrite(u32 regaddr, u32 regvalue, u32 nsize);
extern BOOL SataDsgRegWrite(U32 regaddr, U32 regvalue, U32 nsize);

u32 ugCmdCount = 0;
u32 pifWidth;
u32 dataRamWidth = 0;
bool bigEndian = false;

u8 *g_pIsramBaseAddr[CORE_NUM];
u8 *g_pDsramBaseAddr[CORE_NUM];
u8 *g_pDsramShareBaseAddr;


XTMP_connector g_pifRouter[CORE_NUM];

XTMP_core g_core[CORE_NUM];

regHandleCnf regHandleCnfArr[regHandlerCnfMax] = {0};
XTMP_event simSataDevEvent;
u32 l_ulDevStatus = 0;
u8 g_ucSystemStsSuspendFlag = SYSTEM_RUNING;

// this is only for host model 
void SIM_DevSetStatus(U32 ulStatus)
{
    l_ulDevStatus = ulStatus;
}

// this is only for host model
u32 SIM_DevGetStatus()
{
    return l_ulDevStatus;
}

// this is only for host model
BOOL SIM_DevIsBootUp()
{
    return TRUE;
}


/* call-back function for FW UART printing */
BOOL uartRegWrite(U32 regaddr, U32 regvalue, U32 len)
{
    U8 printVal;

    if( (REG_BASE_UART+4) == regaddr )
    {
        printVal = regvalue&0xFF;
        putchar(printVal);
    }

    return FALSE;
}

/* call-back function for PMU register */
BOOL PmuRegWrite(U32 regaddr, U32 regvalue, U32 len)
{
    if (REG_BASE_PMU == regaddr)// && regaddr < (REG_BASE_PMU + sizeof(U32)))
    {
        if (PMU_INITIATE_SUSP == (PMU_INITIATE_SUSP & regvalue))
        { 
            g_ucSystemStsSuspendFlag = SYSTEM_SUSPENDSTART;
        }
    }
    return true;
}

/* call-back function for FW MCU0 BOOT MCU1/2 */
BOOL BootRegWrite(U32 regaddr, U32 regvalue, U32 len)
{
    U32 ulRegValTemp;
    BOOL bRetFlag = FALSE;
    GLB_REG_MCU tGlobalMcu;
    GLB_REG_MCU tGlobalMcuLocal;
    GLB_REG_SOFTREST tGlobalSoftReset;
    static U32 ulMCU0VecStartIns = 0;
    static U32 ulMCU1VecStartIns = 0;
    static U32 ulMCU2VecStartIns = 0;

    if(regaddr >= (U32)(&rGLB(0x24)) && regaddr < ((U32)(&rGLB(0x24))+sizeof(U32)))
    {
        bRetFlag = TRUE;
    }
    if (regaddr >= (U32)(&rGLB(0x44)) && regaddr < ((U32)(&rGLB(0x44)) + sizeof(U32)))
    {
        ulRegValTemp = regvalue;
        tGlobalMcu = *((GLB_REG_MCU *)&ulRegValTemp);
        Comm_ReadReg((U32)&rGLB(0x44), 1, (U32 *)&tGlobalMcuLocal);
#if 0
        if(tGlobalMcuLocal.bsMcu0RunStall != tGlobalMcu.bsMcu0RunStall)
        {
            if(TRUE == tGlobalMcu.bsMcu0RunStall)
            {
                XTMP_setRunStall(g_core[0], TRUE);
            }
            else
            {
                XTMP_setRunStall(g_core[0], FALSE);
                //store formal instruction, and fill MCU0 ISRAM VECSTART with deadloop instruction
                ulMCU0VecStartIns = *(g_pIsramBaseAddr[0] + ISRAM_REST_VECT_OFFSET);
                *(g_pIsramBaseAddr[0] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;
            }
        }
#endif

        if(tGlobalMcuLocal.bsMCU1RunStall != tGlobalMcu.bsMCU1RunStall)
        {
            if(TRUE == tGlobalMcu.bsMCU1RunStall)
            {
                XTMP_setRunStall(g_core[1], TRUE);
            }
            else
            {
                XTMP_setRunStall(g_core[1], FALSE);
                //store formal instruction, and fill MCU1 ISRAM VECSTART with deadloop instruction
                ulMCU1VecStartIns = *(g_pIsramBaseAddr[1] + ISRAM_REST_VECT_OFFSET);
                *(g_pIsramBaseAddr[1] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;
            }
        }

        if(tGlobalMcuLocal.bsMCU2RunStall != tGlobalMcu.bsMCU2RunStall)
        {
            if(TRUE == tGlobalMcu.bsMCU2RunStall)
            {
                XTMP_setRunStall(g_core[2], TRUE);
            }
            else
            {
                XTMP_setRunStall(g_core[2], FALSE);
                //store formal instruction, and fill MCU2 ISRAM VECSTART with deadloop instruction
                ulMCU2VecStartIns = *(g_pIsramBaseAddr[2] + ISRAM_REST_VECT_OFFSET);
                *(g_pIsramBaseAddr[2] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;
            }
        }

        bRetFlag = TRUE;
    }
    if (regaddr >= (U32)(&rGLB(0x1C)) && regaddr < ((U32)(&rGLB(0x1C)) + sizeof(U32)))
    {
        Comm_ReadReg((U32)&rGLB(0x44), 1, (U32 *)&tGlobalMcuLocal);
         ulRegValTemp = regvalue;
         tGlobalSoftReset = *((GLB_REG_SOFTREST *)&ulRegValTemp);

#if 0    
         if((TRUE == tGlobalMcuLocal.bsRMcu0StatVectorSel) 
             && (FALSE == tGlobalSoftReset.bsRstMcu0) 
             && (FALSE == tGlobalSoftReset.bsRstMcu0If))
         {
             //fill MCU0 ISRAM VECSTART with formal instruction
             *(g_pIsramBaseAddr[0] + ISRAM_REST_VECT_OFFSET) = ulMCU0VecStartIns;
         }
#endif

         if((TRUE == tGlobalMcuLocal.bsRMcu1StatVectorSel) 
             && (FALSE == tGlobalSoftReset.bsRstMcu1) 
             && (FALSE == tGlobalSoftReset.bsRstMcu1If))
         {
             //fill MCU1 ISRAM VECSTART with formal instruction
             *(g_pIsramBaseAddr[1] + ISRAM_REST_VECT_OFFSET) = ulMCU1VecStartIns;
         }

         if((TRUE == tGlobalMcuLocal.bsRMcu2StatVectorSel) 
             && (FALSE == tGlobalSoftReset.bsRstMcu2) 
             && (FALSE == tGlobalSoftReset.bsRstMcu2If))
         {
             //fill MCU2 ISRAM VECSTART with formal instruction
             *(g_pIsramBaseAddr[2] + ISRAM_REST_VECT_OFFSET) = ulMCU2VecStartIns;
         }

        bRetFlag = TRUE;
    }

    return bRetFlag;
}

void PmuClearIDsramOtfbData(void)
{    
    u32 ulTempBuff[OTFB_ALLOCATE_SIZE>>2];
    //clear up Isram,Dsram,Otfb data
    memset((void *)g_pIsramBaseAddr[0], INVALID_2F, ISRAM_REST_VECT_OFFSET);
    memset((void *)g_pIsramBaseAddr[1], INVALID_2F, ISRAM_REST_VECT_OFFSET);
    memset((void *)g_pIsramBaseAddr[2], INVALID_2F, ISRAM_REST_VECT_OFFSET);

    memset((void *)(g_pIsramBaseAddr[0]+ISRAM_REST_VECT_OFFSET+sizeof(u32)), INVALID_2F, MCU0_ISRAM_SIZE-ISRAM_REST_VECT_OFFSET-sizeof(u32));
    memset((void *)(g_pIsramBaseAddr[1]+ISRAM_REST_VECT_OFFSET+sizeof(u32)), INVALID_2F, MCU12_ISRAM_SIZE-ISRAM_REST_VECT_OFFSET-sizeof(u32));
    memset((void *)(g_pIsramBaseAddr[2]+ISRAM_REST_VECT_OFFSET+sizeof(u32)), INVALID_2F, MCU12_ISRAM_SIZE-ISRAM_REST_VECT_OFFSET-sizeof(u32));


    memset((void *)(g_pDsramBaseAddr[0] + DSRAM0_MCU0_OFFSET), INVALID_2F, MCU0_DSRAM0_SIZE);
    memset((void *)(g_pDsramBaseAddr[1] + DSRAM0_MCU1_OFFSET), INVALID_2F, MCU1_DSRAM0_SIZE);
    memset((void *)(g_pDsramBaseAddr[2] + DSRAM0_MCU2_OFFSET), INVALID_2F, MCU2_DSRAM0_SIZE);
    memset((void *)g_pDsramShareBaseAddr, INVALID_2F, MCU012_DSRAM1_SIZE);

    memset((void *)ulTempBuff, INVALID_2F, OTFB_SUSPEND_SIZE);
    dramWrite(OTFB_START_ADDRESS, OTFB_SUSPEND_SIZE>>2, ulTempBuff);

    return;
}

void WaitiCallBack( XTMP_core core, bool waitMode, void* callBackArg )
{
    //true: core entered the wait mode; false:core resumed normal operation
    if(true == waitMode)
    {
        if((core == g_core[1]))
        {
            *(u32 *)(g_pIsramBaseAddr[1] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;
        }

        if((core == g_core[2]))
        {
            *(u32 *)(g_pIsramBaseAddr[2] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;
        }
        XTMP_reset(core);
    }
    return;
}

void NestBootLoaderToFlash(void)
{
    FILE *PFileBootLoader;
    U32 *pBLWriteBuff;
    U32 *PBLReadBuff;
    FLASH_PHY flash_phy;
    flash_phy.nPU = 0;
    flash_phy.nBlock = 0;
    //1. nest bootloader to flash
    PFileBootLoader = fopen("BootLoader.bin", "rb");
    pBLWriteBuff = (U32 *)malloc(FILE_SZ_BOOTLOADER);
    PBLReadBuff = (U32 *)malloc(FILE_SZ_BOOTLOADER);
    fread(pBLWriteBuff, FILE_SZ_BOOTLOADER, 1, PFileBootLoader);
        
    flash_phy.nPage = 0;
    flash_phy.nPln = 0;
    Mid_Write(&flash_phy, RSV_DATA_TYPE, (char*)(pBLWriteBuff), FILE_SZ_BOOTLOADER);
    Mid_Read(&flash_phy, RSV_DATA_TYPE, (char*)(PBLReadBuff), FILE_SZ_BOOTLOADER);

    free(pBLWriteBuff);
    free(PBLReadBuff);
    fclose(PFileBootLoader);
 
    return;
}


void NestFwToFlash(void)
{
    U32 *pFwWriteBuff;
    U32 *pFwReadBuff;
    FILE *PFileFw;
    U32 ulFwPageIndex;
    FLASH_PHY flash_phy;
    flash_phy.nPU = 0;
    flash_phy.nBlock = 0;

    //2. nest FW to flash
    PFileFw = fopen("Fw.bin", "rb");
    pFwWriteBuff = (U32 *)malloc(FILE_SZ_FW);
    pFwReadBuff = (U32 *)malloc(FILE_SZ_FW);
    fread(pFwWriteBuff, FILE_SZ_FW, 1, PFileFw);

    flash_phy.nPage = 1;
    flash_phy.nPln = 0;
    for(ulFwPageIndex = 0; ulFwPageIndex < FILE_SZ_FW/PG_SZ; ulFwPageIndex++)
    {
        Mid_Write(&flash_phy, RSV_DATA_TYPE, (char*)(pFwWriteBuff + ulFwPageIndex*(PG_SZ>>2)), PG_SZ);
        Mid_Read(&flash_phy, RSV_DATA_TYPE, (char*)(pFwReadBuff + ulFwPageIndex*(PG_SZ>>2)), PG_SZ);
        flash_phy.nPage++;
    }

    free(pFwWriteBuff);
    free(pFwReadBuff);
    fclose(PFileFw);
    
    return;
}

void setInterrupt(u32 bit, bool isSet)
{
    XTMP_setInterrupt(g_core[0], bit, isSet);
}

void InsertRegHandleCnf(u32 startAddr, u32 endAddr, regReadHandler regReadFunc, regWriteHandler regWriteFunc)
{
    int i = 0;
    for (i = 0;  i < regHandlerCnfMax; i++)
    {
        if (regHandleCnfArr[i].startAddr == 0 &&
            regHandleCnfArr[i].endAddr ==0 &&
            regHandleCnfArr[i].regReadFunc == 0 &&
            regHandleCnfArr[i].regWriteFunc == 0)
        {

            regHandleCnfArr[i].startAddr = startAddr;
            regHandleCnfArr[i].endAddr = endAddr;
            regHandleCnfArr[i].regReadFunc = regReadFunc;
            regHandleCnfArr[i].regWriteFunc = regWriteFunc;

            break;
        }
    }
};

void handleReadReg(XTMP_address regaddr, u32 regvalue, u32 nsize)
{
    u8 i;

    if(((regaddr >= REG_BASE) && (regaddr < (REG_BASE + DSRAM0_SHARE_TOTAL_SZ)))
        || ((regaddr >= APB_BASE) && (regaddr <= (APB_BASE + APB_SIZE))))
    {
        for (i = 0;  i < regHandlerCnfMax; i++)
        {
            if(regaddr<= regHandleCnfArr[i].endAddr && regaddr >= regHandleCnfArr[i].startAddr)
                if (NULL != regHandleCnfArr[i].regReadFunc)
                    regHandleCnfArr[i].regReadFunc(regaddr, regvalue, nsize);
        }
        return;
    }

#ifdef SIM_FOR_3_CORE
    if((regaddr >=  SRAM0_START_ADDRESS) && (regaddr < ( SRAM0_START_ADDRESS + DSRAM0_SHARE_TOTAL_SZ)))
    {
        printf("FW access reg addr error: %x", (U32)regaddr);
        if (0x1ff81f24 != regaddr)//temp hack
        {
            DBG_Break();
        }
        return;
    }
#endif
}

bool handleWriteReg(XTMP_address regaddr, u32 regvalue, u32 nsize)
{
    u8 i;
    bool btrn = TRUE;
    if(((regaddr >= REG_BASE) && (regaddr < (REG_BASE + DSRAM0_SHARE_TOTAL_SZ))) 
        || ((regaddr >= APB_BASE) && (regaddr <= (APB_BASE + APB_SIZE))))
    {
        for (i = 0;  i < regHandlerCnfMax; i++)
        {
            //if (0x1ff83b3c == regaddr)
            //btrn = FALSE;

            if(regaddr<= regHandleCnfArr[i].endAddr && regaddr >= regHandleCnfArr[i].startAddr)
                if (NULL != regHandleCnfArr[i].regWriteFunc)
                    btrn = regHandleCnfArr[i].regWriteFunc(regaddr, regvalue, nsize);
        }        
    }

#ifdef SIM_FOR_3_CORE
    if((regaddr >= SRAM0_START_ADDRESS) && (regaddr < (SRAM0_START_ADDRESS + DSRAM0_SHARE_TOTAL_SZ)))
    {
        printf("FW access reg addr error: %x", (U32)regaddr);
        DBG_Break();
        btrn = FALSE;
    }
#endif
    return btrn;
}

void DebuggerConnect(void *callBackArg)
{
    //XTMP_userThread sdcThread;
    //printf("debugger connected");
    //XTMP_userThreadNew("sdcthread", sim_satadev_thread, NULL);
#ifdef SIM_FOR_PROFILE
    XTMP_wait(200);
#endif
    XTMP_fireEvent(simSataDevEvent);
    //	XTMP_fireEvent(simFlashStartEvent);
}


void InitMCU(XTMP_params p)
{
    u8 ucCoreIndex;
    u8 aCoreName[10] = "core";
    u32 ucCorePrid[CORE_NUM];
    XTMP_register regPRID;
    
    //InitCoreNum
#ifdef SIM_FOR_3_CORE
    ucCorePrid[0] = MCU0_ID;
    ucCorePrid[1] = MCU1_ID;
    ucCorePrid[2] = MCU2_ID;
#else
    ucCorePrid[0] = MCU1_ID;
#endif

    for(ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
    {
        sprintf((char *)aCoreName, "core%d", ucCoreIndex);
        g_core[ucCoreIndex] = XTMP_coreNew((char *)aCoreName, p, 0);
        if(!g_core[ucCoreIndex])
        {
            fprintf(stderr, "Cannot create XTMP_core cpu%d\n", ucCoreIndex);
            getch();
            exit(1);
        }
        regPRID = XTMP_getRegisterByName(g_core[ucCoreIndex], "PRID"); 
        if (regPRID)
        {
            XTMP_setRegisterValue(regPRID, &ucCorePrid[ucCoreIndex]);
        }
    }

    bigEndian = XTMP_isBigEndian(g_core[0]);
    dataRamWidth = XTMP_loadStoreWidth(g_core[0]);
}

void InitPIFConnecter(XTMP_params p)
{
    u8 ucCoreIndex;
    u8 aPifRouterName[20];
    pifWidth = XTMP_pifWidth(g_core[0]);

    for(ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
    {
        sprintf((char *)aPifRouterName, "pifRouter%d", ucCoreIndex);
        g_pifRouter[ucCoreIndex] = XTMP_connectorNew((char *)aPifRouterName, pifWidth, 0);
        XTMP_connectToCore(g_core[ucCoreIndex], XTMP_PT_PIF, 0, g_pifRouter[ucCoreIndex], 0);
    }
}

void InitNestBootLoaderReglist(void)
{
    FILE *PFileBootLoader;
    u8 *pBLWriteBuff;
    u32 ulBLTextSize;

    PFileBootLoader = fopen("BootLoader.bin", "rb");
    pBLWriteBuff = (u8 *)malloc(FILE_SZ_BOOTLOADER);
    fread(pBLWriteBuff, FILE_SZ_BOOTLOADER, 1, PFileBootLoader);

    ulBLTextSize = 0x3000;
    Comm_WriteOtfb((OTFB_BOOTLOADER_BASE - OTFB_START_ADDRESS + ulBLTextSize), (FILE_SZ_BOOTLOADER - ulBLTextSize)>>2, (u32 *)(pBLWriteBuff + ulBLTextSize));

    free(pBLWriteBuff);
    fclose(PFileBootLoader);

    return;
}

/*
create OTFB, DRAM and trigger register on PIF
*/
void InitSystemMem(XTMP_params p)
{
#if defined(SIM_FOR_3_CORE) && !defined(ROM_BOOTLOADER_FW_FLOW) && !defined(BOOTLOADER_TEST)
    FILE *pFile;
    u8 *pBuff;
#endif

    u8 ucCoreIndex;
    XTMP_device sysPIFReg;
    XTMP_memory sysRam, sysOtfb;
    XTMP_address sysPIFRegAddr, sysRamAddr, sysOtfbAddr;
    u32 sysPIFRegSize, sysRamSize, sysOtfbSize;
    XTMP_deviceFunctions sysPIFRegFuncs = { sysmemPost, sysmemPeek, sysmemPoke };

    //sysRam = DRAM, read/write directly, no call-back function
    sysRamAddr = DRAM_START_ADDRESS;
    sysRamSize = DRAM_ALLOCATE_SIZE;
    sysRam = XTMP_pifMemoryNew("sysRam", pifWidth, bigEndian, sysRamSize);
    XTMP_setReadDelay(sysRam, READ_DELAY);
    XTMP_setWriteDelay(sysRam, WRITE_DELAY);
    XTMP_setBlockReadDelays(sysRam, BLOCK_READ_DELAY, BLOCK_READ_REPEAT);
    XTMP_setBlockWriteDelays(sysRam, BLOCK_WRITE_DELAY, BLOCK_WRITE_REPEAT);

    //sysOtfb = OTFB, read/write directly, no call-back function
    sysOtfbAddr = OTFB_START_ADDRESS;
    sysOtfbSize = OTFB_ALLOCATE_SIZE;//320kB
    sysOtfb = XTMP_pifMemoryNew("sysOtfb", pifWidth, bigEndian, sysOtfbSize);
    XTMP_setReadDelay(sysOtfb, READ_DELAY);
    XTMP_setWriteDelay(sysOtfb, WRITE_DELAY);
    XTMP_setBlockReadDelays(sysOtfb, BLOCK_READ_DELAY, BLOCK_READ_REPEAT);
    XTMP_setBlockWriteDelays(sysOtfb, BLOCK_WRITE_DELAY, BLOCK_WRITE_REPEAT);

    //create PIF register with call-back function
    sysPIFRegSize = APB_SIZE; //36k
    sysPIFRegAddr = APB_BASE;
    sysPIFReg = XTMP_deviceNew("sysPIFReg", &sysPIFRegFuncs, &sysPIFRegData,
        pifWidth, sysPIFRegSize);
    XTMP_setDeviceFastAccessHandler(sysPIFReg, sysmemFastAccess);

    for(ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
    {
        XTMP_addMapEntry(g_pifRouter[ucCoreIndex], sysRam, sysRamAddr, sysRamAddr);
        XTMP_addMapEntry(g_pifRouter[ucCoreIndex], sysOtfb, sysOtfbAddr, sysOtfbAddr);
        XTMP_addMapEntry(g_pifRouter[ucCoreIndex], sysPIFReg, sysPIFRegAddr, sysPIFRegAddr);
    }

#if 0//defined(SIM_FOR_3_CORE) && !defined(ROM_BOOTLOADER_FW_FLOW) && !defined(BOOTLOADER_TEST)
    //1. load binary file to dram
    pFile = fopen("Fw.bin", "rb");
    pBuff = (u8 *)malloc(FILE_SZ_FW);
    fread(pBuff, FILE_SZ_FW, 1, pFile);
    Comm_WriteDram(HEADER_OFFSET, FILE_SZ_FW>>2, (u32 *)pBuff);
    free(pBuff);
    fclose(pFile);
#endif

}

void InitSystemRom(XTMP_params p)
{
    u8 ucCoreIndex;
    u8 aSysRomName[10];
    XTMP_memory aSysRom[CORE_NUM];
    XTMP_address sysRomAddr;
    u32 sysRomSize;

    if (XTMP_getSysRomAddress(p, &sysRomAddr) && 
        XTMP_getSysRomSize(p, &sysRomSize)) 
    {
        sysRomSize = 0x6000;// only 24K rom exists
    }
    else
    {
        getch();
        exit(1);
    }

    for (ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
    {
        sprintf((char *)aSysRomName, "sysRom%d", ucCoreIndex);
        aSysRom[ucCoreIndex] = XTMP_pifMemoryNew((char *)aSysRomName, pifWidth, bigEndian, sysRomSize);
        XTMP_memorySetReadOnly(aSysRom[ucCoreIndex], true);
        XTMP_addMapEntry(g_pifRouter[ucCoreIndex], aSysRom[ucCoreIndex], sysRomAddr, sysRomAddr);
    }
}
void InitLocalIsram(XTMP_deviceFunctions deviceFuncs, u8 CoreId, u8 PortId, XTMP_address IRamAddr, u32 IRamsize)
{
    char IRamName[10] = "IRam";
    XTMP_device IRam[MAX_IRAM_SECTION];
    LocalMemoryData *pLocalMemory = 0;

    pLocalMemory = (LocalMemoryData*)&g_IsramStruct[CoreId];
    pLocalMemory->data = (u8 *) malloc(IRamsize);
    memset(pLocalMemory->data, 0, IRamsize);
    pLocalMemory->base = IRamAddr;
    pLocalMemory->size = IRamsize;
    pLocalMemory->byteWidth = dataRamWidth;
    pLocalMemory->bigEndian = bigEndian;
    pLocalMemory->hasBusy = 0;
    sprintf(IRamName, "IRam%d", CoreId);
    IRam[CoreId] = XTMP_deviceNew((char *)IRamName, &deviceFuncs, pLocalMemory,
        dataRamWidth, IRamsize);
    XTMP_setDeviceFastAccessHandler(IRam[CoreId], memFastAccess);

    g_pIsramBaseAddr[CoreId] = pLocalMemory->data;

    XTMP_connectToCore(g_core[CoreId], XTMP_PT_IRAM, PortId, IRam[CoreId], IRamAddr);
    return;
}

void InitLocalDsram(XTMP_deviceFunctions deviceFuncs, u8 CoreId, u8 PortId, XTMP_address dataRamAddr, u32 dataRamsize)
{
    u8 DataRamName[10] = "DataRam";
    LocalMemoryData *pLocalMemory = 0;
    XTMP_device dataRam;

    pLocalMemory = (LocalMemoryData*)&g_DsramStruct[CoreId];
    pLocalMemory->data = (u8 *) malloc(dataRamsize);
    memset(pLocalMemory->data, 0, dataRamsize);
    pLocalMemory->base = dataRamAddr;
    pLocalMemory->size = dataRamsize;
    pLocalMemory->byteWidth = dataRamWidth;
    pLocalMemory->bigEndian = bigEndian;
    pLocalMemory->hasBusy = 0;
    sprintf((char*)&DataRamName, "DataRam%d", CoreId);
    dataRam = XTMP_deviceNew((char*)DataRamName, &deviceFuncs, pLocalMemory,
        dataRamWidth, dataRamsize);
    XTMP_setDeviceFastAccessHandler(dataRam, memFastAccess);
    XTMP_connectToCore(g_core[CoreId], XTMP_PT_DRAM, PortId, dataRam, dataRamAddr);
    g_pDsramBaseAddr[CoreId] = pLocalMemory->data;
}

void InitLocalShareDsram(XTMP_deviceFunctions deviceFuncs, u8 PortId, XTMP_address dataRamAddr, u32 dataRamsize)
{
    u8 ucCorIndex;
    u8 DataRamName[20] = "DsramShare";
    LocalMemoryData *pLocalMemory = 0;
    XTMP_device dataRam;

    pLocalMemory = (LocalMemoryData*)&g_DsramShareStruct;
    pLocalMemory->data = (u8 *) malloc(dataRamsize);
    memset(pLocalMemory->data, 0, dataRamsize);
    pLocalMemory->base = dataRamAddr;
    pLocalMemory->size = dataRamsize;
    pLocalMemory->byteWidth = dataRamWidth;
    pLocalMemory->bigEndian = bigEndian;
    pLocalMemory->hasBusy = 0;
    dataRam = XTMP_deviceNew((char*)DataRamName, &deviceFuncs, pLocalMemory,
        dataRamWidth, dataRamsize);
    XTMP_setDeviceFastAccessHandler(dataRam, memFastAccess);

    for(ucCorIndex = 0; ucCorIndex < CORE_NUM; ucCorIndex++)
    {
        XTMP_connectToCore(g_core[ucCorIndex], XTMP_PT_DRAM, PortId, dataRam, dataRamAddr);
    }
    g_pDsramShareBaseAddr =  pLocalMemory->data;
}
/*
create DSRAM0/1 and register connected to local port
*/
void InitLocalRam(XTMP_params p)
{
    XTMP_deviceFunctions deviceFuncs = { memPost, memPeek, memPoke };
    XTMP_address dataRamAddr;
    XTMP_address IRamAddr;
    LocalMemoryData *pLocalMemory = 0;
    u8 DataRamName[10] = "DataRam";

    u32 dataRamsize;
    u32 IRamsize;
    u8 uDataRamIndex = 0;
    u8 ucIsramPortId;
    u8 ucDsramPortId;
    u8 ucCoreIndex;

    //------------------------------------------
    //MCU0 isram memory  MCU0/1/2 privacy memory
    ucIsramPortId = 0;
    XTMP_getLocalMemoryAddress(p, XTMP_PT_IRAM, ucIsramPortId, &IRamAddr);
    XTMP_getLocalMemorySize(p, XTMP_PT_IRAM, ucIsramPortId, &IRamsize);

    ucDsramPortId = 0;
    XTMP_getLocalMemoryAddress(p, XTMP_PT_DRAM, ucDsramPortId, &dataRamAddr);
    XTMP_getLocalMemorySize(p, XTMP_PT_DRAM, ucDsramPortId, &dataRamsize);

    for(ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
    {
        InitLocalIsram(deviceFuncs, ucCoreIndex, ucIsramPortId, IRamAddr, IRamsize);
        InitLocalDsram(deviceFuncs, ucCoreIndex, ucDsramPortId, dataRamAddr, dataRamsize);
    }
   
    //MCU012 share Dsram memory
    ucDsramPortId = 1;
    dataRamAddr = dataRamAddr + dataRamsize;
    InitLocalShareDsram(deviceFuncs, ucDsramPortId, dataRamAddr, dataRamsize);
}
//void InitLocalRam(XTMP_params p)
//{
//    XTMP_device dataRam0, dataRam1;
//    XTMP_address dataRam0Addr, dataRam1Addr;
//    u32 dataRam0Size, dataRam1Size;
//    XTMP_deviceFunctions deviceFuncs = { memPost, memPeek, memPoke };
//
//    
//    //create data ram 0 device. Note: reg/prd/trig reg/otfb in this range
//    if (XTMP_getLocalMemoryAddress(p, XTMP_PT_DRAM, 0, &dataRam0Addr) && 
//        XTMP_getLocalMemorySize(p, XTMP_PT_DRAM, 0, &dataRam0Size)) 
//    {
//        dataRam0Struct.data = (u8 *) malloc(dataRam0Size);
//        g_pLocalRam0 = (U8 *)(dataRam0Struct.data);
//        memset(dataRam0Struct.data, 0, dataRam0Size);
//        dataRam0Struct.base = dataRam0Addr;
//        dataRam0Struct.size = dataRam0Size;
//        dataRam0Struct.byteWidth = dataRamWidth;
//        dataRam0Struct.bigEndian = bigEndian;
//        dataRam0Struct.hasBusy = 0;
//
//        /* Create a new device instance and connect it to the data RAM port. */
//        dataRam0 = XTMP_deviceNew("dataRam0", &deviceFuncs, &dataRam0Struct,
//            dataRamWidth, dataRam0Size);
//        
//        XTMP_setDeviceFastAccessHandler(dataRam0, memFastAccess);
//
//        XTMP_connectToCore(core0, XTMP_PT_DRAM, 0, dataRam0, dataRam0Addr);
//    }
//
//    //create data ram 1 device 
//    if (XTMP_getLocalMemoryAddress(p, XTMP_PT_DRAM, 1, &dataRam1Addr) && 
//        XTMP_getLocalMemorySize(p, XTMP_PT_DRAM, 1, &dataRam1Size)) 
//    {
//        dataRam1Struct.data = (u8 *) malloc(dataRam1Size);
//        memset(dataRam1Struct.data, 0, dataRam1Size);
//        dataRam1Struct.base = dataRam1Addr;
//        dataRam1Struct.size = dataRam1Size;
//        dataRam1Struct.byteWidth = dataRamWidth;
//        dataRam1Struct.bigEndian = bigEndian;
//        dataRam1Struct.hasBusy = 0;
//
//        /* Create a new device instance and connect it to the data RAM port. */
//        dataRam1 = XTMP_deviceNew("dataRam1", &deviceFuncs, &dataRam1Struct,
//            dataRamWidth, dataRam1Size);
//        
//        XTMP_setDeviceFastAccessHandler(dataRam1, memFastAccess);
//
//        XTMP_connectToCore(core0, XTMP_PT_DRAM, 1, dataRam1, dataRam1Addr);
//    }
//
//    
//}

#define  G_PAGE_SIZE ((1024*1024*1024)/ PG_SZ)
/*
Init entry for HW model
*/

extern void Host_ModelThread_XTENSA();
extern void AHCI_HOSTCWBQThread_XTENSA();
extern void AHCI_HOSTCFCQThread_XTENSA();
extern void HCT_ModelInit();
extern BOOL AhciRegWriteHandle(U32 regaddr, U32 regvalue, U32 nsize);
extern BOOL DwqDrqRegWrite(U32 regaddr, U32 regvalue, U32 nsize);
extern void SGE_ModelInit(void);
void SIM_LogFileInit();

void InitCostumHwModel(XTMP_params p)
{
    XTMP_userThread HostThread;
    XTMP_userThread wbqThread;
    XTMP_userThread fcqThread;

    SIM_LogFileInit();
    //InitProfileLogVariable();
    SystemStatisticInit();

    ////nfc
    InsertRegHandleCnf(REG_BASE_NDC, REG_BASE_NDC+0x80-1, NULL, (regWriteHandler)nfcRegWrite);
    InsertRegHandleCnf(NFC_CMD_STS_BASE, NFC_CMD_STS_BASE+CE_MAX*(sizeof(NFC_CMD_STS_REG)+sizeof(NF_CQ_REG_CMDTYPE))-1, NULL, (regWriteHandler)nfcRegWrite);
    InsertRegHandleCnf(TRIGGER_REG_BASE, TRIGGER_REG_BASE+NFCQ_DEPTH*CE_MAX*sizeof(TRIGGER_REG_BASE)-1, NULL, (regWriteHandler)nfcRegWrite);
    InsertRegHandleCnf(PUACC_TRIG_REG_BASE, PUACC_TRIG_REG_BASE+sizeof(U32)-1, NULL, (regWriteHandler)nfcRegWrite);

    //sge
    InsertRegHandleCnf((U32)(&rDsgReportMcu1), (U32)(&rDsgReportMcu2)+sizeof(U32)-1, NULL, (regWriteHandler)NormalDsgRegWrite); 
    InsertRegHandleCnf((U32)(&rDsgReportMcu0), (U32)(&rDsgReportMcu0)+sizeof(U32)-1, NULL, (regWriteHandler)NormalDsgRegWrite); 
    //InsertRegHandleCnf((U32)(&rHsgReportMcu1), (U32)(&rHsgReportMcu2)+sizeof(U32)-1, NULL, (regWriteHandler)HsgRegWrite); 
    //InsertRegHandleCnf((U32)(&rHsgReportMcu0), (U32)(&rHsgReportMcu0)+sizeof(U32)-1, NULL, (regWriteHandler)HsgRegWrite);

    //InsertRegHandleCnf((U32)REG_BASE_HCT, (U32)(REG_BASE_HCT + 0x100), NULL, (regWriteHandler)AhciRegWriteHandle);

    //InsertRegHandleCnf((U32)(&rDrqMcu1Status), (U32)(&rDwqMcu2Status) + sizeof(u32) - 1, NULL, (regWriteHandler)DwqDrqRegWrite);
    //InsertRegHandleCnf((U32)(&rChainNumMcu1), (U32)(&rChainNumMcu2) + sizeof(u32) - 1, NULL, (regWriteHandler)DwqDrqRegWrite);
    //InsertRegHandleCnf((U32)(&rNfcChainNumMcu1), (U32)(&rNfcChainNumMcu2) + sizeof(u32) - 1, NULL, (regWriteHandler)DwqDrqRegWrite);

    ////uart printing
    InsertRegHandleCnf(REG_BASE_UART, REG_BASE_UART+0x10, NULL, (regWriteHandler)uartRegWrite);

    ////performance record
    InsertRegHandleCnf((U32)&rTraceData1, ((U32)&rTraceData1)+0x20, NULL, (regWriteHandler)SystemPerformanceRecord);

    //mcu0 boot mcu1/2
    InsertRegHandleCnf((U32)(&rGLB(0x24)), (U32)(&rGLB(0x24)) + sizeof(U32) - 1, NULL, (regWriteHandler)BootRegWrite);
    InsertRegHandleCnf((U32)(&rGLB(0x44)), (U32)(&rGLB(0x44)) + sizeof(U32) - 1, NULL, (regWriteHandler)BootRegWrite);
    InsertRegHandleCnf((U32)(&rGLB(0x1C)), (U32)(&rGLB(0x1C))+sizeof(U32)-1, NULL, (regWriteHandler)BootRegWrite); 

    //PMU
    InsertRegHandleCnf(REG_BASE_PMU, REG_BASE_PMU+sizeof(U32)-1, NULL, (regWriteHandler)PmuRegWrite);

    // register user thread
    //flash model
    Comm_NFCModelInit();

    ////host/sdc model
    Host_ModelInit();
    HostThread = XTMP_userThreadNew("satahost", (XTMP_userThreadFunction)Host_ModelThread_XTENSA, NULL);

    //Comm_NFCModelInit();

#if defined (HOST_SATA)
    Comm_NormalDsgModelInit();
    Comm_SataDsgModelInit();
    SDC_ModelInit();
	    //SDC register
    InsertRegHandleCnf(REG_BASE_SDC, REG_BASE_DRAMC - 1, sdcRegRead, (regWriteHandler)sdcRegWrite);
    InsertRegHandleCnf(REG_BASE_BUFM, REG_BASE_BUFM + 0x10 -1 , sdcRegRead, (regWriteHandler)sdcRegWrite);
    InsertRegHandleCnf((U32)(&rSataDsgReportWrite1), (U32)(&rAvailableWriteDsgCnt) + sizeof(U32) - 1, NULL, (regWriteHandler)SataDsgRegWrite);

    XTMP_userThreadNew("SDC", (XTMP_userThreadFunction)SDC_ModelThread_XTENSA, NULL);
#endif
#ifndef SIM_FOR_3_CORE
    //MV_CmdInit();
#endif

    ////DMAE model
    DMAE_ModelInit();
    InsertRegHandleCnf(DMAE_CMDENTRY_BASE, DMAE_CMDENTRY_BASE + sizeof(DMAE_CMDENTRY) * DMAE_CMDENTRY_COUNT, NULL, (regWriteHandler)DmaeRegWrite);

    ////search engine
    SE_ModelInit();
    InsertRegHandleCnf((U32)(&rSE_TRIGGER_SIZE_REG(SE0)), (U32)(&rSE_TRIGGER_SIZE_REG(SE0)) + 1, NULL, (regWriteHandler)SERegWrite);
    InsertRegHandleCnf((U32)(&rSE_TRIGGER_SIZE_REG(SE1)), (U32)(&rSE_TRIGGER_SIZE_REG(SE1)) + 1, NULL, (regWriteHandler)SERegWrite);
    InsertRegHandleCnf((U32)(&rSE_TRIGGER_SIZE_REG(SE2)), (U32)(&rSE_TRIGGER_SIZE_REG(SE2)) + 1, NULL, (regWriteHandler)SERegWrite);

    //HCT_ModelInit();
    //wbqThread = XTMP_userThreadNew("wbqthread", (XTMP_userThreadFunction)AHCI_HOSTCWBQThread_XTENSA, NULL);
    //fcqThread = XTMP_userThreadNew("fcqthread", (XTMP_userThreadFunction)AHCI_HOSTCFCQThread_XTENSA, NULL);
    //SGE_ModelInit();
}
#ifdef SIM_FOR_3_CORE
//for wholechip
#define LOAD_ELF_PATH_MCU0 \
    "D:\\GIT\\SSD-VT3514-3\\Bin\\bin\\SATA_MCU0_L85_B0_XTMP.elf"
#define LOAD_ELF_PATH_MCU1 \
    "D:\\GIT\\SSD-VT3514-3\\Bin\\bin\\SATA_MCU12_L85_B0_XTMP.elf"
#define LOAD_ELF_PATH_MCU2 LOAD_ELF_PATH_MCU1
#else
//for mixvector
#define LOAD_ELF_PATH_MCU1 \
    "D:\\GIT\\SSD-VT3514-3\\Bin\\bin\\MIXVECTOR_MCU12_L85_B0_XTMP.elf"
#endif
#define XTENSA_SYSTEM "C:/usr/xtensa/XtDevTools/install/tools/RE-2014.5-win32/XtensaTools/config"

int XTMP_main(int argc, char **argv)
{
    char *xtensaSystemDirs[] = { XTENSA_SYSTEM, 0 };
    XTMP_params p;
    xtmp_options options;
    XTMP_userThread tickerThread;
    u32 aPort[CORE_NUM];
    u8 ucCoreIndex;

    //event used by DebuggerConnect()
    simSataDevEvent = XTMP_eventNew();

    init_options(&options);
    if (get_options(&options, argc, argv) != 0) {
        exit(1);
    }

    p = XTMP_paramsNewFromPath(xtensaSystemDirs, XTENSA_CORE, 0);
    if (!p) 
    {
        fprintf(stderr, "Cannot create XTMP_params "XTENSA_CORE"\n");
        getch();
        exit(1);
    }

    InitMCU(p);
    InitPIFConnecter(p);
    InitSystemMem(p);
    InitSystemRom(p);
    InitLocalRam(p);
    InitCostumHwModel(p);

    SIM_XtBootLoader(FALSE);

    if (options.turbo) 
    {
        for (ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
        {
            if (XTMP_switchSimMode(g_core[ucCoreIndex], XTMP_FUNCTIONAL) != XTMP_DEVICE_OK) 
            {
                fprintf(stderr, "Could not switch cpu to functional mode\n");
            }
        }
    }
    else 
    {
        //simulation in cycle-accurate mode, sysmemTicker will be waken up @every cycle
        tickerThread = XTMP_userThreadNew("ticker", sysmemTicker, &sysPIFRegData);
    }

    for (ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
    {
        if (options.trace_file[0])
        {
            XTMP_setTraceFile(g_core[ucCoreIndex], options.trace_file);
        }

        if (options.trace_level >= 0)
        {
            XTMP_setTraceLevel(g_core[ucCoreIndex], options.trace_level);
        }
    }

#ifdef SIM_FOR_3_CORE        
    //load tensilica ELF file 
    if ((!XTMP_loadProgram(g_core[0], LOAD_ELF_PATH_MCU0, NULL))
        || (!XTMP_loadProgram(g_core[1], LOAD_ELF_PATH_MCU1, NULL))
        || (!XTMP_loadProgram(g_core[2], LOAD_ELF_PATH_MCU2, NULL)))
#else
    if (!XTMP_loadProgram(g_core[0], LOAD_ELF_PATH_MCU1, NULL))
#endif
    {
        fprintf(stderr, "Cannot load %s\n", options.prog_name);
        getch();
        exit(1);
    }
#ifdef ROM_BOOTLOADER_FW_FLOW
    NestBootLoaderToFlash();
    NestFwToFlash();
#endif

#ifdef BOOTLOADER_TEST
    NestFwToFlash();
    InitNestBootLoaderReglist();
#endif
    //use command line arguments "--debug=20000" to enable debug
    if (options.enable_debug) 
    {
        for (ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
        {
            aPort[ucCoreIndex] = XTMP_enableDebug(g_core[ucCoreIndex], options.debug_port);
            if (!options.xxdebug)
            {
                fprintf(stderr, "Waiting for debugger on port %d\n", aPort[ucCoreIndex]);
            }

            //start executing instructions only when the debugger has connected
            XTMP_setWaitForDebugger(g_core[ucCoreIndex], true);
            XTMP_setDebuggerConnectCallBack(g_core[ucCoreIndex], DebuggerConnect, NULL);
        }
    }
    else
    {
        XTMP_userThreadNew("waitcore", DebuggerConnect, NULL);
    }


    //controls the number of cycles of work that a core can execute out of order
    XTMP_setRelaxedSimulationCycleLimit(options.ooo_cycles);  
#ifdef SIM_FOR_3_CORE
    XTMP_setWaitiCallBack( g_core[1], (XTMP_waitiCallBack)WaitiCallBack, NULL);
    XTMP_setWaitiCallBack( g_core[2], (XTMP_waitiCallBack)WaitiCallBack, NULL);

#ifdef BOOTLOADER_TEST
    XTMP_setStatVectorSel(g_core[0], 1);
    XTMP_reset(g_core[0]);
#endif

#if !defined(BOOTLOADER_TEST) && !defined(ROM_BOOTLOADER_FW_FLOW)
    *(u32 *)(g_pIsramBaseAddr[0] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;
    XTMP_setStatVectorSel(g_core[0], 1);
    XTMP_reset(g_core[0]);
#endif

    //set mcu1/2 boot from isram
    for (ucCoreIndex = 1; ucCoreIndex < CORE_NUM; ucCoreIndex++)
    {
        //2. set mcu1/2 vect start instuction while(1);
        *(u32 *)(g_pIsramBaseAddr[ucCoreIndex] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;

        //set vect stary from isram
        XTMP_setStatVectorSel(g_core[ucCoreIndex], 1);
        XTMP_reset(g_core[ucCoreIndex]);
    }
#else
        //set vect start from isram
        XTMP_setStatVectorSel(g_core[0], 1);
        XTMP_reset(g_core[0]);
#endif

#ifdef SIM_FOR_PROFILE
    while (ugCmdCount <126)
    {
        XTMP_stepSystem(1);
    }
#else
    XTMP_stepSystem(options.cycle_limit);
#endif

    if (options.summary)
    {
        for (ucCoreIndex = 0; ucCoreIndex < CORE_NUM; ucCoreIndex++)
        {
            XTMP_printSummary(g_core[ucCoreIndex], false);
        }
    }

    if ( options.turbo && !strcmp(options.prog_name, "sieve.out") )
        fprintf(stderr, "\nNote: To see the full benefit of TurboXim, "
        "run with --program=sieve_long.out\n\n");

    XTMP_cleanup();

    return 0;
}

LOCAL char l_strLogFolder[256] = { 0 };
void SIM_LogFileInit()
{
    char strLogFolder[] = "\\WinLog";

    _getcwd(l_strLogFolder, sizeof(l_strLogFolder));
    strcat(l_strLogFolder, strLogFolder);

    if (0 != _access(l_strLogFolder, 0))
    {
        if (0 != _mkdir(l_strLogFolder))
        {
            DBG_Printf("There have error id = %d, info = \n", GetLastError());
        }
    }
}

char* SIM_GetLogFileFloder()
{
    return l_strLogFolder;
}