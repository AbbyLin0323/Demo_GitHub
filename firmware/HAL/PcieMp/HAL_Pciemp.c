/* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
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
Filename    : HAL_Pciemp.h
Version     : Ver 1.0
Author      : John
Date        : 2017.08.17
Description : this file declare pciemp function
Modify      :

*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"

#ifndef SIM
#include <xtensa/config/core.h>
#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>
#include <xtensa/tie/xt_core.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/tie/xt_timer.h>
#include <xtensa/tie/via.h>
extern U32 _bss_end;//the variable '_bss_end' is defined by Xtensa Linker
#else
#include "xt_library.h"
#endif
#include "HAL_Pciemp.h"
#include "HAL_Interrupt.h"
#include "HAL_ParamTable.h"
#include "L0_ViaCmd.h"
LOCAL volatile MP_REG_SET * l_pMpRegSet;
GLOBAL volatile U32 l_ulRevMpInt;
PCIE_MP_STAT gPcieStat;
extern U32 g_ulATARawBuffStart;
#ifndef SATA
extern volatile U32 g_PmCtrMpMode;
#endif
void HAL_MpGlbSetting(void)
{
    rHOSTC(0x38) = 0;          // disable opt rom
    rGLB(0x18) |= 0x20000000;  //disable pcie
    HAL_DelayCycle(2);         //wait for reset completed
    rGLB(0x18) &= ~0x20000000; // enable pcie [29]
#if 0
p/x * 0x1ff8003c = 0x4
p/x * 0x1ff80040 |= 0x1<<27
p/x * 0x1ff800a0 |= 0x1<<18

p/x * 0x1ff80018 = 0
p/x * 0x1ff81f1c |= (1<<8)
p/x * 0x1ff83a38 = 0
p/x * 0x1ff80068 |= (1<<18)
p/x * 0x1ff83a50 &= (~1<<10)
p/x * 0x1ff837a4 |= (1<<24)
#endif
    /* design remove */
    rGLB(0x3C) = 0x4;

    /**
     * PIF send ERROR address process
     * 1: Response MCU ERROR
     * 0: No response
     */
    rGLB(0x40) |= 0x1<<27;
    /* MCU send all  write requests in the queue RMCU_REQ_FLU */
    rGLB(0xA0) |= 0x1<<18;

    /* design remove */
    rGLB(0x68) |= (1<<18);
#ifdef FPGA
    /* 1'b1:Enable PCIE outband RST in */
    rPMU(0x1C) |= (1<<8);
#endif

    /**
     * PCIe Low-Power Mode Enable. Setting to 0 makes PCIE IP stays
     * in L0 when host setting D3 state (Not entering PML1)
     */
    rHOSTC(0x50) &= (~1<<10);

    /**
     * PCIe F0 does not check Mem Hit. When setting this, PCIE IP not check the MemAddr
     * range with PCIE_BAR. This should be removed in final released version since the
     * design has been ECOed.
     */
    /*
    *(volatile U32 *)0x1ff837a4 |= (1<<24);
    */
}

void HAL_PcieMpInit()
{
    gPcieStat = STAT_ISSUE_CMD;
    l_pMpRegSet = (volatile MP_REG_SET *)REG_BASE_NVME_MP;

    l_ulRevMpInt = FALSE;
    l_pMpRegSet->bsCmdTrig = TRUE;

    DBG_Printf("Pcie Mp Init222 Done!\n");
    return;
}

void HAL_PcieMpProc()
{
#ifndef SATA
    VIA_CMD_PARAM *pViaSubCmdParam;
    U32 ulSubCmdType = l_pMpRegSet->bsCmdType;
    U32 aBuf[3];
    VIA_CMD_STATUS ulStatus;
    U32 ulMemCnt;
    U32 ulFlashStatusOffset;
    
    if(0 != l_ulRevMpInt)
    {
        //DBG_Printf("HAL_PcieMpProc ulSubCmdType %d start!\n", ulSubCmdType);
        pViaSubCmdParam = (VIA_CMD_PARAM *)&l_pMpRegSet->ulParam[0];
        ulMemCnt = (pViaSubCmdParam->tMemAccess.bsByteLen >> 2);
        switch (gPcieStat)
        {
            case STAT_ISSUE_CMD:
            {

                if (OP_DISABLE_PMU == ulSubCmdType)
                {
                    g_PmCtrMpMode = TRUE;
                }
                else if (OP_ENABLE_PMU == ulSubCmdType)
                {
                    g_PmCtrMpMode = FALSE;
                }
                else
                {
                    aBuf[0]=0;
                    aBuf[1]=0;                  
                    aBuf[2]=0;                                      
                    ulStatus = L0_ViaHostCmd(0, ulSubCmdType, pViaSubCmdParam, aBuf);
                    //DBG_Printf("HAL_PcieMpProc ulSubCmdType %d Done, ulStatus %d aBuf %d %d %d !\n", ulSubCmdType, ulStatus, aBuf[0], aBuf[1], aBuf[2]);
                    l_pMpRegSet->bsCmdStatus = ulStatus;
                    
                    if (VIA_CMD_MEM_WRITE == ulSubCmdType)
                    {
                       // DBG_Printf("VIA SCMD VIA_CMD_MEM_WRITE\n");
                        // DO NOTHING
                    }
                    else if (VIA_CMD_MEM_READ == ulSubCmdType)
                    {
                        //DBG_Printf("VIA SCMD VIA_CMD_MEM_READ\n");
                        
                        COM_MemCpy((U32 *)&l_pMpRegSet->ulParam[0], g_ulATARawBuffStart, ulMemCnt);
                        
                    }
                    else if ((ulSubCmdType >= VIA_CMD_FLASH_READ) && (ulSubCmdType <= VIA_CMD_FLASH_ERASE))
                    {
                        DBG_Printf("VIA SCMD FLASH OPERATION\n");
    //                    l_tTxPack.ulDWord[0] = HAL_UartDmaTx((U32*)g_ulATARawBuffStart,SUB_SYSTEM_PU_MAX);
                
                        gPcieStat = STAT_SEND_STAT;
                    }
                    
                    else
                    {
                        //DBG_Printf("VIA SCMD NONE DATA\n");
                        COM_MemCpy((U32 *)&l_pMpRegSet->ulParam[0], aBuf, 3);     
                        
                    }
                }
                
                break;
            }

            case STAT_SEND_STAT:
            {
                ulFlashStatusOffset = l_pMpRegSet->ulParam[0];
                ulMemCnt = l_pMpRegSet->ulParam[1];
                
                DBG_Printf("VIA SCMD FLASH STATUS Dwoff:0x%x Dwcnt:%d\n",ulFlashStatusOffset,ulMemCnt);
                
                if(INVALID_8F == ulFlashStatusOffset)
                {
                    gPcieStat = STAT_ISSUE_CMD;
                }
                else
                {
                    COM_MemCpy((U32 *)&l_pMpRegSet->ulParam[0], g_ulATARawBuffStart + (ulFlashStatusOffset << 2), ulMemCnt);
                    gPcieStat = STAT_SEND_STAT;
                }
                break;
            }

            default:
                break;
        }
        
        l_pMpRegSet->bsCmdTrig = TRUE;
        l_ulRevMpInt = FALSE;
    }
#endif
}

