/****************************************************************************
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
*****************************************************************************
* File Name    : MixVector_Main.c
* Discription  : main loop of mix vector; implement process flow of host command
and common interface
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/
#include "BaseDef.h"
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "MixVector_Interface.h"
#include "MixVector_SRAM.h"
#include "MixVector_DRAM.h"
#include "MixVector_Flash.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_NormalDSG.h"
#include "HAL_HSG.h"
#include "HAL_SGE.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_HwDebug.h"
#ifdef COSIM
#include "HAL_DramConfig.h"
#endif

#define CDC_WAIT_TIME 10    // NOP instructions inserted in order to wait for hardware to update some interface register

extern GLOBAL MCU12_VAR_ATTR U8 *g_pSubSystemPUMapping;
extern U32 ulSendLen[MAX_SLOT_NUM];
extern U32 ulRemLen[MAX_SLOT_NUM];
extern U32 ulIndex[MAX_SLOT_NUM];
GLOBAL HOST_CMD_MANAGER g_aHostCmdManager[MAX_SLOT_NUM];
GLOBAL HOST_CMD *g_pHCmdTable;
GLOBAL MCU12_VAR_ATTR U32 g_ulSSU0Base;
extern GLOBAL MCU12_VAR_ATTR U32 g_ulRedDataBase;

LOCAL U32 l_ulRecieveCmdCnt = 0;

extern void TEST_SEMain(void);
extern void TEST_DMAEMain(void);
extern void TEST_DataCheck_EM(void);
#ifndef SIM
volatile U32 g_ulDbgEnable;

/****************************************************************************
Name        :DBG_Getch
Input       :None
Output      :None
Author      :HenryLuo
Date        :2012.02.15    15:11:36
Description :Recording context and pending MCU when a fatal error is encountered.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT DBG_Getch()
{
    U32 ulTestLoop = 0;
    DBG_Printf("Fatal Error, DBG_Getch!!!\n");

    while (g_ulDbgEnable)
    {
        ulTestLoop++;
    }
    g_ulDbgEnable = 1;
}
#endif

/*------------------------------------------------------------------------------
Name: MixVectorInit
Description: 
initialize global variable, CST status, base address, reset Flash, etc.
Input Param:
void
Output Param:
none
Return Value:
void
Usage:
initialize FW/HW state before main loop run.
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
void MixVectorInit(void)
{
    U32 ulFreeSramBase = DSRAM1_MCU1_BASE;
    
    g_ulRedDataBase = OTFB_RED_DATA_MCU1_BASE;

    //g_pSubSystemPUMapping = (U8 *)ulFreeSramBase;
#ifdef COSIM
    COM_MemIncBaseAddr(&ulFreeSramBase, (sizeof(U8)*PU_PER_SUBSYSTEM));
    COM_MemAddr16DWAlign(&ulFreeSramBase);
#endif
    HAL_HwDebugInit();

    /*
        if we do sub module test, we skip the following sequence
        and the sub module test code should do related initialization
    */
#ifndef SUB_MODEL_TEST
#if defined(COSIM) //only COSIM ENV need FW code to initialize DDR
    HAL_DramcInit();
#endif
    InitHCTReg();
    HAL_FlashInit();//pu mapping need init first before call HalFlashDriverInit()
    //HalNfcPuScheduleInit();
    //HalFlashDriverInit();//reset all pu in FlashDriver Init;
    HAL_NormalDsgInit();
    HAL_HsgInit();
    HAL_DrqInit();
    HAL_DwqInit();
    HAL_SgeInitChainCnt();
    HAL_NfcInitChainCnt();

    //clear host command manager to initial value
    COM_MemZero((U32*)&g_aHostCmdManager[0], sizeof(g_aHostCmdManager)/sizeof(U32));

    g_pCacheStatus = (U8 *)OTFB_CACHE_STATUS_MCU1_BASE;
    g_pSSU1 = (U8 *)OTFB_SSU1_MCU1_BASE;
    rNfcSsu1Base = OTFB_SSU1_MCU1_BASE - OTFB_START_ADDRESS;
#endif
#if defined(FPGA)
    *(volatile U32 *)(REG_BASE_GLB + 0x28) = 0x12800608;
    *(volatile U32 *)(REG_BASE_GLB + 0x2C) = 0x0E629062;
    *(volatile U32 *)(0x1FF83A40) = 0x80;
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: CalcAndSetChainNum
Description: 
calculate total chain num and set it to SGE. as FW run in single core,
we will set chain num of MCU2 to zero
Input Param:
U8 ucCmdSlot: command slot index
Output Param:
none
Return Value:
U16: chain number for NFC on-the-fly write
Usage:
if need to enable WBQ wait SGE data done, FW call this function to determine
total chain num.
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
LOCAL U16 CalcAndSetChainNum(U8 ucCmdSlot)
{
    U16 usChainCnt = 0;
    U16 usNfcChainCnt = 0;
    U16 usStartPage, usEndPage;
    HOST_CMD *pHostCmd;

    pHostCmd = &g_pHCmdTable[ucCmdSlot];

    // calc diskB chain(DRQ/DWQ) count
    if (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_B].DiskEn)
    {
        // StartUnit is in units of sector (512 bytes, 0.5k bytes)
        // should be align to SSD Page, which is 32k bytes.
        // so right shift by 6 bits.
        usStartPage = pHostCmd->SubDiskCmd[SUB_DISK_B].StartUnit >> SEC_PER_BUF_BITS;
        usEndPage = (pHostCmd->SubDiskCmd[SUB_DISK_B].StartUnit
            + pHostCmd->SubDiskCmd[SUB_DISK_B].UnitLength - 1) >> SEC_PER_BUF_BITS;

        usChainCnt += ( usEndPage - usStartPage + 1);
    }

    // calc diskC chain(SGQ) count: we do not calculate erase command as no data transfering in such command.
    if (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_C].DiskEn
        && HOST_REQ_READ == pHostCmd->SubDiskCmd[SUB_DISK_C].ReqType)
    {
        usStartPage = pHostCmd->SubDiskCmd[SUB_DISK_C].StartUnit >> SEC_PER_BUF_BITS;
        usEndPage = (pHostCmd->SubDiskCmd[SUB_DISK_C].StartUnit
            + pHostCmd->SubDiskCmd[SUB_DISK_C].UnitLength - 1) >> SEC_PER_BUF_BITS;

        usChainCnt += ( usEndPage - usStartPage + 1);
    }

    // calc diskC on-the-fly program chain(SGQ with write type) count
    if (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_C].DiskEn
        && HOST_REQ_WRITE == pHostCmd->SubDiskCmd[SUB_DISK_C].ReqType)
    {
        usStartPage = pHostCmd->SubDiskCmd[SUB_DISK_C].StartUnit >> SEC_PER_BUF_BITS;
        usEndPage = (pHostCmd->SubDiskCmd[SUB_DISK_C].StartUnit
            + pHostCmd->SubDiskCmd[SUB_DISK_C].UnitLength - 1) >> SEC_PER_BUF_BITS;

        usNfcChainCnt += ( usEndPage - usStartPage + 1);
    }

    //set NFC chain count firstly to sync with C model behavior
    HAL_NfcFinishChainCnt(ucCmdSlot, usNfcChainCnt);
    HAL_NfcHelpFinishChainCnt(ucCmdSlot);
#if defined(FPGA)    
    DBG_Printf("CalcAndSetChainNum(), usNfcChainCnt = %d\n", usNfcChainCnt);
    DBG_Printf("CalcAndSetChainNum(), usChainCnt = %d\n", usChainCnt);
#endif    
    // set total chain count to SGE
    HAL_SgeFinishChainCnt(ucCmdSlot, usChainCnt);
    //help to set total chain number to 0 for MCU2
    HAL_SgeHelpFinishChainCnt(ucCmdSlot);

    return usNfcChainCnt;
}

/*------------------------------------------------------------------------------
Name: PreProcessHostCmd
Description: 
pre-process host command: initialize HOST_CMD_MANAGER and build WBQ for write diskA
Input Param:
U8 ucCmdSlot: command slot index
Output Param:
none
Return Value:
void
Usage:
when command received, FW call this function to pre-process host command.
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
LOCAL void PreProcessHostCmd(U8 ucCmdSlot)
{
    HOST_CMD *pHostCmd;
    HOST_CMD_MANAGER *pHostCmdMgr;
    U16 usNfcChainCnt;
    U8 i;
    SUB_DISK_CMD *tSubDiskBCmd;
    SUB_DISK_CMD *tSubDiskCCmd;
    SUB_DISK_CMD *tSubDiskBCmdMgr;
    SUB_DISK_CMD *tSubDiskCCmdMgr;

    pHostCmd = &g_pHCmdTable[ucCmdSlot];
    pHostCmdMgr = &g_aHostCmdManager[ucCmdSlot];

    if ( g_pHCmdTable[ucCmdSlot].FinishCnt[SUB_DISK_B] > 0 )
    {
        DBG_Getch();
    }

    tSubDiskBCmd = &g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B];
    tSubDiskCCmd = &g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_C];
    tSubDiskBCmdMgr = &g_aHostCmdManager[ucCmdSlot].SubDiskBCmd;
    tSubDiskCCmdMgr = &g_aHostCmdManager[ucCmdSlot].SubDiskCCmd;

    COM_MemCpy((U32 *) tSubDiskBCmdMgr,(U32 *)tSubDiskBCmd,(sizeof(SUB_DISK_CMD)/sizeof(U32)));
    COM_MemCpy((U32 *) tSubDiskCCmdMgr,(U32 *)tSubDiskCCmd,(sizeof(SUB_DISK_CMD)/sizeof(U32)));

    g_aHostCmdManager[ucCmdSlot].ContinueHSGDiskB = INVALID_4F;
    g_aHostCmdManager[ucCmdSlot].ContinueHSGDiskC = INVALID_4F;
    for(i = 0; i < SUB_DISK_CNT; i++)
    {
        g_aHostCmdManager[ucCmdSlot].StartUnitInByte[i] = g_pHCmdTable[ucCmdSlot].SubDiskCmd[i].StartUnit << SEC_SIZE_BITS;
        g_aHostCmdManager[ucCmdSlot].UnitLenthInByte[i] = g_pHCmdTable[ucCmdSlot].SubDiskCmd[i].UnitLength<< SEC_SIZE_BITS;

        if(SUB_DISK_C == i)
        {
            g_DiskCHSGMonitor[ucCmdSlot].HostAddrHigh = g_pHCmdTable[ucCmdSlot].SubDiskCmd[i].HostAddrHigh;
            g_DiskCHSGMonitor[ucCmdSlot].HostAddrLow = g_pHCmdTable[ucCmdSlot].SubDiskCmd[i].HostAddrLow;
        }
    }
    // No matter WBQ wait SGE or not, we should always tell SGE that how many chain it needs to do.
    // Otherwise, SGE can not know it finish a tag or not.
    usNfcChainCnt = CalcAndSetChainNum(ucCmdSlot);
    if (0 != usNfcChainCnt)
    {
        pHostCmdMgr->WbqWaitNfcEn = TRUE;
    }
    else
    {
        pHostCmdMgr->WbqWaitNfcEn = FALSE;
    }
    
    /* judge write request to disk A */
    if (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_A].DiskEn
        && HOST_REQ_WRITE == pHostCmd->SubDiskCmd[SUB_DISK_A].ReqType)
    {
        pHostCmdMgr->WaitCstEn = TRUE;
    }

    /* check if there is ERASE command to diskC or not */
    if (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_C].DiskEn
        && HOST_REQ_ERASE == pHostCmd->SubDiskCmd[SUB_DISK_C].ReqType)
    {
        pHostCmdMgr->Ssu1En = TRUE;
        pHostCmdMgr->WbqWaitSgeEn = FALSE;
        if (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_B].DiskEn)
        {
            pHostCmdMgr->CacheStatusEn = TRUE;
        }
    }
    else if (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_B].DiskEn
        || (TRUE == pHostCmd->SubDiskCmd[SUB_DISK_C].DiskEn && HOST_REQ_READ == pHostCmd->SubDiskCmd[SUB_DISK_C].ReqType))
    {
        pHostCmdMgr->WbqWaitSgeEn = TRUE;

        if (FALSE == pHostCmdMgr->WaitCstEn)
        {
            ReturnDiskADataAndCompleteHostCmd(ucCmdSlot, TRUE, pHostCmdMgr->WbqWaitNfcEn);
        } 
    }
    else
    {
        /* no access to disk B/C, or only write diskC by on-the-fly path */
        pHostCmdMgr->WbqWaitSgeEn = FALSE;

        if (FALSE == pHostCmdMgr->WaitCstEn)
        {
        #if defined(FPGA)
            DBG_Printf("slot %d, pHostCmdMgr->WbqWaitNfcEn = %d\n", ucCmdSlot, pHostCmdMgr->WbqWaitNfcEn);
        #endif            
            ReturnDiskADataAndCompleteHostCmd(ucCmdSlot, FALSE, pHostCmdMgr->WbqWaitNfcEn);
        }
    }

    return;
}
void ClearDiskABMem()
{
    U32 addr;
    for(addr = HCT_DISK_A_BASE;addr < ((24*1024));addr+=4)
    {
        *(volatile U32 *)addr = 0;
    }

    for(addr = 0x42000000;addr < ((128*1024*1024));addr+=4)
    {
        *(volatile U32 *)addr = 0;
    }
}
extern U32 ulSendLen[MAX_SLOT_NUM];
extern U32 ulRemLen[MAX_SLOT_NUM];
extern U32 ulIndex[MAX_SLOT_NUM];
U8 ucCurCmdSlot = 0;
int MixVectorHandleHCmd(void)
{
    U8 ucCmdSlot;
    BOOL bDiskBProcessDone, bDiskCProcessDone;
    HOST_CMD *pHostCmd;
    HOST_CMD_MANAGER *pHostCmdMgr;
    volatile U32* pxCI;

    BOOL bJumpOutFlag = FALSE;

    pxCI = (volatile U32 *) ( REG_BASE_AHCI + 0x138 );
    for (ucCmdSlot = 0; ucCmdSlot < MAX_SLOT_NUM; ucCmdSlot++)
    {
        //error handling
        if (TRUE == l_bNfcErr)
        {
            ProcessNfcError(l_ucErrPU);
            l_bNfcErr = FALSE;
        }

        pHostCmd = &g_pHCmdTable[ucCmdSlot];
        pHostCmdMgr = &g_aHostCmdManager[ucCmdSlot];
        switch (pHostCmdMgr->Stage)
        {
        case HCMD_STAGE_INIT:
            if ( ( CST_STATUS_INIT == GetCST( ucCmdSlot ) ) )
            {
                if ( ( (*pxCI) & ( 1 << ucCmdSlot ) ) == ( 1 << ucCmdSlot ) )
                {            

//                    DBG_Printf("RECV CMD ucCmdSlot:%d\n",ucCmdSlot);
                    ulSendLen[ucCmdSlot] = 0;
                    ulRemLen[ucCmdSlot] = 0;
                    ulIndex[ucCmdSlot] = 0;
                    if ( CST_STATUS_INIT != GetCST( ucCmdSlot ) )
                    {
                        DBG_Getch();
                    }
                    //clear to initial value
                    COM_MemZero( (U32*)pHostCmdMgr, sizeof( HOST_CMD_MANAGER ) / sizeof( U32 ) );

                    HAL_HwDebugStart(ucCmdSlot);
                    if (TRUE == FetchHostCmd(ucCmdSlot))
                    {
                        pHostCmdMgr->Stage = HCMD_STAGE_WAIT_CMD;
                        l_ulRecieveCmdCnt++;
                        if (0 == l_ulRecieveCmdCnt % MAX_SLOT_NUM)
                        {
                            DBG_Printf("recivec cmd count %d\n", l_ulRecieveCmdCnt);
                        }                            
                    }
                    else
                    {
                        //DBG_Break();
                    }
                }
            }
            break;
            
        case HCMD_STAGE_WAIT_CMD:
            if (CST_STATUS_CMD_RCV == GetCST( ucCmdSlot ) )
            {
                PreProcessHostCmd(ucCmdSlot);

                if (TRUE == pHostCmdMgr->WaitCstEn)
                {
                    pHostCmdMgr->Stage = HCMD_STAGE_BUILD_FCQ;
                }
                else
                {
                    pHostCmdMgr->Stage = HCMD_STAGE_PROCESS;
                }
            }
            break;

            /* specail for write diskA request */
        case HCMD_STAGE_BUILD_FCQ:
            if (TRUE == ProcessWriteDiskA(ucCmdSlot))
            {
                pHostCmdMgr->Stage = HCMD_STAGE_WAIT_FCQ_DATA_DONE;
            }
            break;

        case HCMD_STAGE_WAIT_FCQ_DATA_DONE:
            if (CST_STATUS_DATA_DONE == GetCST( ucCmdSlot ) )
            {
                pHostCmdMgr->Stage = HCMD_STAGE_PROCESS;

                if (TRUE == pHostCmdMgr->WbqWaitSgeEn || TRUE == pHostCmdMgr->WbqWaitNfcEn)
                {
                    ReturnDiskADataAndCompleteHostCmd(ucCmdSlot, pHostCmdMgr->WbqWaitSgeEn, pHostCmdMgr->WbqWaitNfcEn);
                }
                else if (FALSE == pHostCmdMgr->Ssu1En && FALSE == pHostCmdMgr->CacheStatusEn)
                {
                    ReturnDiskADataAndCompleteHostCmd(ucCmdSlot, FALSE, FALSE);
                }
            }
            break;

        case HCMD_STAGE_PROCESS:
            bDiskBProcessDone = ProcessDiskB(ucCmdSlot);
            bDiskCProcessDone = ProcessDiskC(ucCmdSlot);
            if (TRUE == bDiskBProcessDone && TRUE == bDiskCProcessDone)
            {
                if (TRUE == pHostCmdMgr->Ssu1En || TRUE == pHostCmdMgr->CacheStatusEn)
                {
                    pHostCmdMgr->Stage = HCMD_STAGE_WAIT_COMPLETE;
                }
                else
                {
                    pHostCmdMgr->Stage = HCMD_STAGE_INIT;
                }
#if 0
                //caution: MAX_SLOT_NUM below should change to max number of host send cmd slot. 
                //eg:g_ucCmdNum = 4; in HostModel.c MAX_SLOT_NUM below should instead by 4;
                if(ucCmdSlot == (MAX_SLOT_NUM - 1))
                {
                    ucCurCmdSlot = 0;
                }
#endif                
            }
            else
            {
                //ucCurCmdSlot = ucCmdSlot;
                bJumpOutFlag = TRUE;
            }
            break;

        case HCMD_STAGE_WAIT_COMPLETE:
            if (TRUE == pHostCmdMgr->Ssu1En && FALSE  == CheckSsuBusy(ucCmdSlot))
            {
                break;
            }

            if (TRUE == pHostCmdMgr->CacheStatusEn && FALSE  == CheckCacheStatusBusy(ucCmdSlot))
            {
                break;
            }

            //tobey 
            pHostCmdMgr->Stage = HCMD_STAGE_INIT;
           // DBG_Printf("cmd %d finish \n");
            ulSendLen[ucCmdSlot] = 0;
            ulRemLen[ucCmdSlot] = 0;
            ulIndex[ucCmdSlot] = 0;
            DBG_Printf("cmd %d finish \n",ucCmdSlot);
            ReturnDiskADataAndCompleteHostCmd(ucCmdSlot, FALSE, FALSE);
            break;

        default:
            //can not get here
            DBG_Getch();
            break;
        }

        if(TRUE == bJumpOutFlag)
        {
            break;
        }
    }

    return 0;
}

void HAL_SubModelTest(void)
{
#ifdef FLASH_DRIVER_TEST_LOW_LEVEL
    TEST_NfcMain();
#endif

#ifdef SE_TEST
    TEST_SEMain();
#endif

#ifdef DMAE_TEST
    TEST_DMAEMain();
#endif

#ifdef SPINLOCK_TEST
    Test_SpinLockMain();
#endif

#ifdef MCU_BASIC_FUNC_TEST
    Test_McuBasicFuncMain();
    Test_McuInterruptMain();
#endif

#ifdef HOST_REQUEST_TEST
    HAL_HostReqTest();
#endif

#ifdef EM_DATACHECK_TEST
    TEST_DataCheck_EM();
#endif

#ifdef SPI_TEST
    TEST_SpiMain();
#endif

#ifdef EFUSE_TEST
    HAL_EfuseTestMain();
#endif

    return;
}

int MV_Schedule(void)
{
#ifdef SUB_MODEL_TEST
    HAL_SubModelTest();
#else
    //handle MixVector protocol
    MixVectorHandleHCmd();
#endif

    return 0;
}

#ifndef SIM
#if defined(FPGA)
extern U8 g_printf_flag;
#endif
int main(void)
{
    g_ulDbgEnable = 1;
    #if defined(FPGA)
    g_printf_flag = 1;
    COM_MemZero((U32 *)(0x1ffd8000),32*24);
    uart_init();
    DBG_Printf("uart init done!!\n");
    #endif
#ifndef SUB_MODEL_TEST
    MixVectorInit();
#endif
#if defined(FPGA)
    *(U32 *)(REG_BASE_GLB + 0x54) = 0x00F0E0;
    *(U32 *)(REG_BASE_NDC + 0x20) = 0x0000aaaa;//DBG_MN
    *(U32 *)(REG_BASE_NDC + 0x24) = 0x00080008;
    *(U32 *)(REG_BASE_NDC + 0x28) = 0x00080008;

 /*   *(U32 *)(REG_BASE_GLB + 0x54) = 0x003130;//HCT DBG_GRP_HCLK_0
    *(U8 *)(REG_BASE_HCT + 0x7c) = 0x0;*/
    COM_MemZero(ulSendLen,MAX_SLOT_NUM);
    COM_MemZero(ulRemLen,MAX_SLOT_NUM);
    COM_MemZero(ulIndex,MAX_SLOT_NUM);    
    ClearDiskABMem();

#endif
    while(1)
    {
        MV_Schedule();
    }  

    return 0;
}
#endif
/* end of file MixVectorMain.c */
