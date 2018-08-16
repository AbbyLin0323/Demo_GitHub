/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc. and may  *
* contain trade secrets and/or other confidential information of VIA           *
* Technologies, Inc. This file shall not be disclosed to any third party, in   *
* whole or in part, without prior written consent of VIA.                      *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : BBT_Test.c
* Discription  :
* CreateAuthor : Steven
* CreateDate   : 2015.5.12
*===============================================================================
* Modify Record:
* 2015.5.27 Jason Refactoring
*=============================================================================*/
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_Dmae.h"
#include "L0_Interface.h"
#include "L1_SCmdInterface.h"
#include "L2_TableBBT.h"
#include "BBT_Test.h"

#define BBT_UT_SUBSYSTEM_NUM 1


LOCAL  U32 l_ulBbtLoopCnt = 0;
LOCAL  U32 l_ulDbgGBBT;
LOCAL  U32 l_ulDbgLBBT;
LOCAL  U8 l_ucSavedTLun;
LOCAL U32 l_ulChangeTestModeSync[2] = { 0 };

extern GLOBAL  PSCMD g_pCurSCmd;

///////////////////////////////////////////////////////////////////////////////////
// Dbg Bbt-memory-bits interfaces
///////////////////////////////////////////////////////////////////////////////////

// Bbt Test : set and get the test-global-bbt-bit
LOCAL void MCU1_DRAM_TEXT L2_BbtTestSetGBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBytePos;

    ulBytePos = l_ulDbgGBBT + (ucTLun * BBT_BLK_PER_PLN * PLN_PER_LUN + ucPln * BBT_BLK_PER_PLN + usBlock) / 8;
    *(volatile U8 *)ulBytePos |= (1 << (usBlock % 8));

    return;
}
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtTestGetGBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBytePos;
    BOOL bBadBlk;

    ulBytePos = l_ulDbgGBBT + (ucTLun * BBT_BLK_PER_PLN * PLN_PER_LUN + ucPln * BBT_BLK_PER_PLN + usBlock) / 8;
    bBadBlk = (0 != (*(volatile U8 *)ulBytePos & (1 << (usBlock % 8)))) ? TRUE : FALSE;

    return bBadBlk;
}

// Bbt Test : set and get the test-local-bbt-bit
LOCAL void MCU1_DRAM_TEXT L2_BbtTestSetLBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBytePos;

    ulBytePos = l_ulDbgLBBT + ucTLun * LBBT_BUF_SIZE + (ucPln * BBT_BLK_PER_PLN + usBlock) / 8;
    *(volatile U8 *)ulBytePos |= (1 << (usBlock % 8));

    return;
}
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtTestGetLBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBytePos;
    BOOL bBadBlk;

    ulBytePos = l_ulDbgLBBT + ucTLun * LBBT_BUF_SIZE + (ucPln * BBT_BLK_PER_PLN + usBlock) / 8;
    bBadBlk = (0 != (*(volatile U8 *)ulBytePos & (1 << (usBlock % 8)))) ? TRUE : FALSE;

    return bBadBlk;
}

// Bbt Test : check the blk is bad or not in the test-global-bbt and the test-local-bbt
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtTestIsGBbtBadBlock(U8 ucTLun, U16 usPhyBlk)
{
    U8 ucPln;
    BOOL bBadBlk = FALSE;

    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        if (TRUE == L2_BbtTestGetGBbtBadBlkBit(ucTLun, ucPln, usPhyBlk))
        {
            bBadBlk = TRUE;
            break;
        }
    }

    return bBadBlk;
}
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtTestIsLBbtBadBlock(U8 ucTLun, U16 usPhyBlk)
{
    U8 ucPln;
    BOOL bBadBlk = FALSE;

    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        if (TRUE == L2_BbtTestGetLBbtBadBlkBit(ucTLun, ucPln, usPhyBlk))
        {
            bBadBlk = TRUE;
            break;
        }
    }

    return bBadBlk;
}

// add the bad blk to the test-global-bbt and test-local-bbt
GLOBAL void MCU1_DRAM_TEXT L2_BbtTestAddBadBlk(U8 ucTLun, U16 usPhyBlk)
{
    U8 ucPln;

    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        L2_BbtTestSetGBbtBadBlkBit(ucTLun, ucPln, usPhyBlk); // global-bit
        L2_BbtTestSetLBbtBadBlkBit(ucTLun, ucPln, usPhyBlk); // local-bit
    }

    return;
}

LOCAL void MCU1_DRAM_TEXT L2_BbtTestPrintBbt(U8 ucTLun)
{
    U8 ucPln;
    U16 usBlk;
    U32 ulBadBlkCnt;

    ulBadBlkCnt = 0;
    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        for (usBlk = 0; usBlk < BBT_BLK_PER_PLN; usBlk++)
        {
            if (TRUE == L2_BbtTestGetGBbtBadBlkBit(ucTLun, ucPln, usBlk))
            {
                ulBadBlkCnt++;
                DBG_Printf("MCU#%d LUN:%d PLN:%d BLK:%d.\n", HAL_GetMcuId(), ucTLun, ucPln, usBlk);
            }
        }
    }

    DBG_Printf("MCU#%d BBT_TEST LUN:%d TotalBadBlkCnt %d.\n", HAL_GetMcuId(), ucTLun, ulBadBlkCnt);

    return;
}
LOCAL void MCU1_DRAM_TEXT L2_BbtTestPrintAllBbt(void)
{
    U8 ucTLun;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        L2_BbtTestPrintBbt(ucTLun);
    }

    return;
}
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtTestChkBbtData(void)
{
    U8  ucTLun, ucPln;
    U16 usBlk;
    BOOL bBadBlk1, bBadBlk2, bBadBlk3;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (usBlk = GBBT_BLK + 1; usBlk < BBT_BLK_PER_PLN; usBlk++)
        {
            for(ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
            {
                bBadBlk1 = L2_BbtIsGBbtBadBlock(ucTLun, usBlk);
                bBadBlk2 = L2_BbtTestIsGBbtBadBlock(ucTLun, usBlk);
                bBadBlk3 = L2_BbtTestIsLBbtBadBlock(ucTLun, usBlk);
                
                if (bBadBlk1 != bBadBlk2 || bBadBlk1 != bBadBlk3 || bBadBlk2 != bBadBlk3 )
                {
                    DBG_Printf("MCU#%d FunMode#1 TLun%d Blk%d bbt-sts:%d dbgbbt-sts:%d_%d\n", HAL_GetMcuId(), ucTLun, usBlk, bBadBlk1, bBadBlk2, bBadBlk3);
                    DBG_Getch();
                }
            }
        }
    }
    
    return TRUE;
}

// generate a dummy bad blk
LOCAL U8 MCU1_DRAM_TEXT L2_BbtTestGenerateBadBlk(void)
{
    U8  ucTLun;
    U16 usBlk;
    U8 ucErrBlk, ucErrType;

    ucTLun = rand() % SUBSYSTEM_LUN_NUM;
    usBlk = rand() % BLK_PER_PLN;

    if (usBlk < GBBT_BLK + 1)
    {
        return INVALID_2F;
    }

    if (TRUE == L2_BbtIsGBbtBadBlock(ucTLun, usBlk))
    {
        return INVALID_2F;
    }
    
    ucErrBlk = (rand() % 3) + 1;
    ucErrType = rand() % 3;
    L2_BbtAddBbtBadBlk(ucTLun, usBlk, ucErrBlk, ucErrType);
    //L2_BbtTestAddBadBlk(ucTLun, usBlk);

    DBG_Printf("MCU#%d TLun#%d Blk#%d Add to Dummy Bad Blk.\n", HAL_GetMcuId(), ucTLun, usBlk);

    return ucTLun;
}

GLOBAL void MCU1_DRAM_TEXT L2_BbtTestDramAllocate(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;

    // Allocate the global-bbt buffer, only one global-bbt buffer.
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulDbgGBBT = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, GBBT_BUF_SIZE);

    // Allocate the local-bbt buffer, each local-bbt has a separate buffer.
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulDbgLBBT = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, LBBT_BUF_SIZE_TOTAL);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    *pFreeDramBase = ulFreeDramBase;

    return;
}

LOCAL void MCU1_DRAM_TEXT L2_BbtTestMemZero(void)
{
    HAL_DMAESetValue(l_ulDbgGBBT, COM_MemSize16DWAlign(GBBT_BUF_SIZE), 0);

    HAL_DMAESetValue(l_ulDbgLBBT, COM_MemSize16DWAlign(LBBT_BUF_SIZE_TOTAL), 0);

    return;
}

LOCAL void MCU1_DRAM_TEXT L2_BbtTestInit(void)
{
    L2_BbtTestMemZero();

    return;
}

LOCAL BOOL MCU1_DRAM_TEXT L2_BbtTestSimLLFBoot(void)
{
    BOOL bLLFBOOTDone = FALSE;

    g_pCurSCmd = L1_GetSCmd();
    if (NULL != g_pCurSCmd)
    {
        if (SCMD_LLF == g_pCurSCmd->ucSCmdType)
        {
            DBG_Printf("\nMCU#%d BBT FAKE SCMD LLF done!\n", HAL_GetMcuId());
        }

        if (SCMD_BOOTUP == g_pCurSCmd->ucSCmdType)
        {
            bLLFBOOTDone = TRUE;
            DBG_Printf("\nMCU#%d BBT FAKE SCMD BOOTUP done!\n", HAL_GetMcuId());
        }

        L1_SCmdFinish();
    }

    return bLLFBOOTDone;
}

/*==============================================================================
Func Name  : L2_BbtTest
Input      : None
Output     : None
Return Val :
Discription: BBT Test
Usage      :
History    :
1. 2015.04.29 StevenChang create function
2. 2015.05.27 Jason Refactoring
==============================================================================*/
BOOL L3_BbtTest(void)
{
    LOCAL U8 l_TestBBTStatus = BBT_UT_INIT;
    
    switch (l_TestBBTStatus)
    {
        case BBT_UT_INIT:
        {
            if (TRUE == L2_BbtTestSimLLFBoot())
            {
                l_TestBBTStatus = BBT_UT_LLF_FORMAT_BBT;
                DBG_Printf("MCU#%d BBT UnitTest Enter to First-LLF-With-FormatBBT Mode.\n", HAL_GetMcuId());
            }
            break;
        }
        case BBT_UT_LLF_FORMAT_BBT:
        {
            if (BBT_UT_SUBSYSTEM_NUM == 2)
            {
                if (l_ulChangeTestModeSync[0] > l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU1_ID)
                {
                    break;
                }
                if (l_ulChangeTestModeSync[0] < l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU2_ID)
                {
                    break;
                }
            }
            
            DiskConfig_SetLLFMethold(LLF_METHOD_FORMAT_BBT);

            L2_BbtTestInit();

            l_TestBBTStatus = BBT_UT_BUILD;
            DBG_Printf("MCU#%d BBT UnitTest BBT-Init-FormatBBT -> BBT-Build.\n", HAL_GetMcuId());
            break;
        }
        case BBT_UT_LLF_READ_IDB:
        {
            if (BBT_UT_SUBSYSTEM_NUM == 2)
            {
                if (l_ulChangeTestModeSync[0] > l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU1_ID)
                {
                    break;
                }
                if (l_ulChangeTestModeSync[0] < l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU2_ID)
                {
                    break;
                }
            }
            DiskConfig_SetLLFMethold(LLF_METHOD_READ_IDB);

            L2_BbtTestInit();

            l_TestBBTStatus = BBT_UT_BUILD;
            DBG_Printf("MCU#%d BBT UnitTest BBT-Init-ReadIDB -> BBT-Build.\n", HAL_GetMcuId());
            break;
        }
        case BBT_UT_LLF_REDETECT_IDB:
        {
            if (BBT_UT_SUBSYSTEM_NUM == 2)
            {
                if (l_ulChangeTestModeSync[0] > l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU1_ID)
                {
                    break;
                }
                if (l_ulChangeTestModeSync[0] < l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU2_ID)
                {
                    break;
                }
            }
            DiskConfig_SetLLFMethold(LLF_METHOD_REDETECT_IDB);

            L2_BbtTestInit();

            l_TestBBTStatus = BBT_UT_BUILD;
            DBG_Printf("MCU#%d BBT UnitTest BBT-Init-ReDetectIDB -> BBT-Build.\n", HAL_GetMcuId());
            break;
        }
        case BBT_UT_LLF_NORMAL:
        {
            if (BBT_UT_SUBSYSTEM_NUM == 2)
            {
                if (l_ulChangeTestModeSync[0] > l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU1_ID)
                {
                    break;
                }
                if (l_ulChangeTestModeSync[0] < l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU2_ID)
                {
                    break;
                }
            }
            DiskConfig_SetLLFMethold(LLF_METHOD_NORMAL);
                       
            l_TestBBTStatus = BBT_UT_BUILD;
            DBG_Printf("MCU#%d BBT UnitTest BBT-Init-Normal -> BBT-Build.\n", HAL_GetMcuId());
            break;
        }
        case BBT_UT_LLF_FORMAT_GBBT:
        {
            if (BBT_UT_SUBSYSTEM_NUM == 2)
            {
                if (l_ulChangeTestModeSync[0] > l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU1_ID)
                {
                    break;
                }
                if (l_ulChangeTestModeSync[0] < l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU2_ID)
                {
                    break;
                }
            }
            DiskConfig_SetLLFMethold(LLF_METHOD_FORMAT_GBBT);
            
            l_TestBBTStatus = BBT_UT_BUILD;
            DBG_Printf("MCU#%d BBT UnitTest BBT-Init-RebuildBBT -> BBT-Build.\n", HAL_GetMcuId());
            break;
        }
        case BBT_UT_BOOT:
        {
            if (BBT_UT_SUBSYSTEM_NUM == 2)
            {
                if (l_ulChangeTestModeSync[0] > l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU1_ID)
                {
                    break;
                }
                if (l_ulChangeTestModeSync[0] < l_ulChangeTestModeSync[1] && HAL_GetMcuId() == MCU2_ID)
                {
                    break;
                }
            }
            
            l_TestBBTStatus = BBT_UT_LOAD;
            DBG_Printf("MCU#%d BBT UnitTest BBT-Init-Boot -> BBT-Boot.\n", HAL_GetMcuId());
            break;
        }
        case BBT_UT_BUILD:
        {
            if (TRUE == L2_BbtBuild(FALSE))
            {
                l_TestBBTStatus = BBT_UT_LOAD;
                DBG_Printf("MCU#%d BBT UnitTest BBT-Build -> BBT-Load.\n", HAL_GetMcuId());
            }
            break;
        }
        case BBT_UT_LOAD:
        {
            if (TRUE == L2_BbtLoad(NULL))
            {
                l_TestBBTStatus = BBT_UT_CHECK;
                DBG_Printf("MCU#%d BBT UnitTest BBT-Load -> BBT-Check.\n", HAL_GetMcuId());
            }
            break;
        }
        case BBT_UT_CHECK:
        {
            if (FALSE == L2_BbtTestChkBbtData())
            {
                L2_BbtPrintAllBbt();
                L2_BbtTestPrintAllBbt();

                DBG_Printf("MCU#%d BBT UnitTest BBT_UT_CHECK Bbt-Data-Mis-Match.\n", HAL_GetMcuId());
                DBG_Getch();
            }

            l_ulBbtLoopCnt++;
            if (0 == l_ulBbtLoopCnt % BBT_UT_CHANGE_MODE_THRD)
            {
                l_TestBBTStatus = BBT_UT_FINISH;
                DBG_Printf("MCU#%d BBT UnitTest BBT-Check-Pass -> Update Test Mode.\n", HAL_GetMcuId());
            }
            else
            {
                l_TestBBTStatus = BBT_UT_ADD_DUMY_BAD_BLK;
                DBG_Printf("MCU#%d BBT UnitTest BBT-Check-Pass -> Add Dummy Bad Blk.\n", HAL_GetMcuId());
            }
            
            break;
        }
        case BBT_UT_ADD_DUMY_BAD_BLK:
        {
            l_ucSavedTLun = L2_BbtTestGenerateBadBlk();
            if (INVALID_2F != l_ucSavedTLun)
            {
                l_TestBBTStatus = BBT_UT_SAVE;
                DBG_Printf("MCU#%d BBT UnitTest Add Dummy Bad Blk -> BBT-Save.\n", HAL_GetMcuId());
            }
            break;
        }
        case BBT_UT_SAVE:
        {
            if (TRUE == L2_BbtSave(l_ucSavedTLun, INVALID_2F))
            {
                l_TestBBTStatus = BBT_UT_LOAD;
                DBG_Printf("MCU#%d BBT UnitTest BBT-Save -> BBT-Load.\n", HAL_GetMcuId());
                //L2_BbtTestPrintAllBbt();
            }
            break;
        }
        case BBT_UT_FINISH:
        {
            // LLF-uninherit -> add x bad blks -> LLF-inherit-fw-bbt -> add x bad blks -> Boot -> add x bad blks -> LLF-uninherit -> ...
            if (1 * BBT_UT_CHANGE_MODE_THRD == l_ulBbtLoopCnt % BBT_UT_CHANGE_MODE_STEP)
            {
                l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]++;
                l_TestBBTStatus = BBT_UT_LLF_READ_IDB;
                DBG_Printf("MCU#%d BBT UnitTest Enter to LLF-Inherit-ReadIDB Mode 0x%x.\n", HAL_GetMcuId(), l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]);                                
                break;
            }

            if (2 * BBT_UT_CHANGE_MODE_THRD == l_ulBbtLoopCnt % BBT_UT_CHANGE_MODE_STEP)
            {                
                l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]++;
                l_TestBBTStatus = BBT_UT_LLF_NORMAL;
                DBG_Printf("MCU#%d BBT UnitTest Enter to LLF-Inherit-Normal Mode 0x%x.\n", HAL_GetMcuId(), l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]);
                break;
            }

            if (3 * BBT_UT_CHANGE_MODE_THRD == l_ulBbtLoopCnt % BBT_UT_CHANGE_MODE_STEP)
            {
                l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]++;
                l_TestBBTStatus = BBT_UT_BOOT;
                DBG_Printf("MCU#%d BBT UnitTest Enter to Boot Mode 0x%x.\n", HAL_GetMcuId(), l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]);
                break;
            }
            
            if (4 * BBT_UT_CHANGE_MODE_THRD == l_ulBbtLoopCnt % BBT_UT_CHANGE_MODE_STEP)
            {
                l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]++;
                l_TestBBTStatus = BBT_UT_LLF_FORMAT_GBBT;
                DBG_Printf("MCU#%d BBT UnitTest Enter to LLF-Inherit-RebuildBBT Mode 0x%x.\n", HAL_GetMcuId(), l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]);
                break;
            }

            if (5 * BBT_UT_CHANGE_MODE_THRD == l_ulBbtLoopCnt % BBT_UT_CHANGE_MODE_STEP)
            {
                l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]++;
                l_TestBBTStatus = BBT_UT_LLF_REDETECT_IDB;
                DBG_Printf("MCU#%d BBT UnitTest Enter to LLF-Inherit-ReDetectIDB Mode 0x%x.\n", HAL_GetMcuId(), l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]);
                break;
            }

            if (0 == l_ulBbtLoopCnt % BBT_UT_CHANGE_MODE_STEP)
            {
                l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]++;
                l_TestBBTStatus = BBT_UT_LLF_FORMAT_BBT;
                DBG_Printf("MCU#%d BBT UnitTest Enter to LLF-UnInherit-FormatBBT Mode 0x%x.\n", HAL_GetMcuId(), l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]);
                break;
            }

            DBG_Printf("MCU#%d BBT UnitTest Stage BBT_UT_FINISH LoopCnt = %d 0x%x.\n", HAL_GetMcuId(), l_ulBbtLoopCnt, l_ulChangeTestModeSync[HAL_GetMcuId() - MCU1_ID]);
            DBG_Getch();
        }
        default:
        {
            DBG_Printf("MCU#%d BBT UnitTest Stage Error = %d .\n", HAL_GetMcuId(), l_TestBBTStatus);
            DBG_Getch();
        }
    }

    return FALSE;
}

