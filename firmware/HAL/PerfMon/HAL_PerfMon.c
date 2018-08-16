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
Filename    : HAL_PerfMon.c
Version     : Ver 1.0
Author      : AbbyLin
Date        : 2017.03.28
Description : this file encapsulate Trax driver interface. 
Others      : 
Modify      :
20170328    Abby     Create file
*******************************************************************************/
#if (defined(FPGA) || defined(ASIC))

#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_PerfMon.h"

#include <xtensa/tie/xt_externalregisters.h>

/*------------------------------------------------------------------------------
    GLOBAL VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL BOOL g_bPerfMRdCnt, g_bPerfMClrCnt;  //if need to read without interrupt, set g_bPerfMRdCnt = 1
GLOBAL volatile U32 g_aPerfMCnt[PM_COUNT_MAX] = {0};
GLOBAL volatile U32 g_ulOvflCnt;    //over flow counter in FW

//add for PM of L2
#ifdef PM_TEST
GLOBAL volatile unsigned int g_ulPMCurTime, g_ulDiffCycle;
GLOBAL volatile U32 g_aPMWCnt[WRITE_TYPE_NUM] = {0};
GLOBAL volatile U32 g_aPMWCntOvfl[WRITE_TYPE_NUM] = {0};
#endif

/*------------------------------------------------------------------------------
    FUNCTIONS DEFINITION
------------------------------------------------------------------------------*/
void HAL_PerfMSetCntlReg(U8 ucCntIdx, U8 ucEvent, U16 usSubEvent)
{
    U32 ulRegAddr = (U32)PM_CTRL_REG_BASE + ucCntIdx*sizeof(PM_CNTL_REG);
    PM_CNTL_REG tCntlReg = {0};
    
    tCntlReg.bIntEn = FALSE;//TRUE;
    tCntlReg.bKrnlCnt = TRUE;
    tCntlReg.bTraceLevel = 0;
    tCntlReg.bSelect = ucEvent;
    tCntlReg.bMask = usSubEvent;

    XT_WER(*(U32*)&tCntlReg, ulRegAddr);
    
    return;
}

LOCAL void HAL_PerfMEnableCounting(void)
{
    PM_GLOBAL_REG tGlbReg = {0};

    tGlbReg.bPMEn = TRUE;
    XT_WER(*(U32*)&tGlbReg, (U32)PMG_REG);
    
    return;
}

LOCAL void HAL_PerfMDisableCounting(void)
{
    PM_GLOBAL_REG tGlbReg = {0};

    tGlbReg.bPMEn = FALSE;
    XT_WER(*(U32*)&tGlbReg, (U32)PMG_REG);
    
    return;
}

/* check if there is interrupts and return the type */
PM_INT_TYPE HAL_PerfMGetInterruptType(U8 ucCntIdx)
{
    PM_INT_TYPE eIntType = PM_NONE_INT;
    U32 ulStsRegAddr = (U32)PM_STAT_REG_BASE + ucCntIdx * sizeof(PM_STAT_REG);
    volatile PM_STAT_REG tStatus = {0};

    *(U32*)&tStatus = XT_RER(ulStsRegAddr);
    
    if (tStatus.bOvfl)
        eIntType = PM_OVFL_INT;

    if (tStatus.bIntAsrt)
        eIntType |= PM_INTASRT; //if both int is set, will return PM_ALL_INT = PM_OVFL_INT | PM_INTASRT

    return eIntType;
}

/* clearing these bits does not disable counting */
U32 HAL_PerfMClrInterrupt(U8 ucCntIdx, PM_INT_TYPE eIntType)
{
    U32 ulStatus = FALSE;
    U32 ulStsRegAddr = (U32)PM_STAT_REG_BASE + ucCntIdx * sizeof(PM_STAT_REG);
    volatile PM_STAT_REG tStatus = {0};

    if (PM_OVFL_INT == eIntType)
    {
        tStatus.bOvfl = TRUE;
        XT_WER(*(U32*)&tStatus, ulStsRegAddr);
        
        *(U32*)&tStatus = XT_RER(ulStsRegAddr);
        ulStatus = (0 == tStatus.bOvfl)? TRUE : FALSE; //TRUE: clear successful
    }        

    if (PM_INTASRT == eIntType)
    {
        tStatus.bIntAsrt = TRUE;
        XT_WER(*(U32*)&tStatus, ulStsRegAddr);

        *(U32*)&tStatus = XT_RER(ulStsRegAddr);
        ulStatus = (0 == tStatus.bIntAsrt)? TRUE : FALSE; //TRUE: clear successful
    }//if both int is set, will be clear all when execute here

    return ulStatus;
}

U32 HAL_PerfMGetIntPC(void)
{
    return XT_RER((U32)INTPC_REG);
}

//return INT_PC, pending
U32 HAL_PerfMHandleInterrupt(U8 ucCntIdx, PM_INT_TYPE eIntType)
{
    U32 ulIntPC;
    
    if (PM_NONE_INT == eIntType)
    {   
        return;//no interrupt need to handle
    }

    ulIntPC = HAL_PerfMGetIntPC();
    DBG_Printf("Interrupt happened in PM, type %d IntPC 0x%x\n", eIntType, ulIntPC);

    //only hangle over flow interrupt now
    if (TRUE == (PM_OVFL_INT & eIntType))
    {
        g_ulOvflCnt++;
    }
}

void HAL_PerfMSetUp(U8 ucCntIdx, U32 ulCntPreValue, U8 ucSel, U16 usMask)
{   
    //check and clear interrupt
    if (PM_NONE_INT != HAL_PerfMGetInterruptType(ucCntIdx))//exist interrupt
    {
        DBG_Printf("PM Setup: Not all interrupt are clear!\n");
        DBG_Getch();
    }

    //set up PM control
    HAL_PerfMSetCntlReg(ucCntIdx, ucSel, usMask);

    //PM values must be written to if required  
    if (INVALID_8F != ulCntPreValue)
    {
        U32 ulRegAddr = (U32)PM_COUNT_BASE + ucCntIdx * sizeof(U32);
        XT_WER(ulCntPreValue, ulRegAddr);
    }

    return;
}   

void HAL_PerfMStartCounting(void)
{    
    //enable PM counting and wait 10 cycles at least
    HAL_PerfMEnableCounting();
    HAL_DelayCycle(10);

    return;
}

void HAL_PerfMInit(void)
{
    //disable PM counting before setup counters
    HAL_PerfMDisableCounting();

    //init global reg in FW
    g_ulOvflCnt = 0;
    g_bPerfMRdCnt = 0;
    g_bPerfMClrCnt= 0;
    
    return;
}

U32 HAL_PerfMGetCount(U8 ucCntIdx)
{   
    U32 ulRegAddr = (U32)PM_COUNT_BASE + ucCntIdx * sizeof(U32);

    return XT_RER(ulRegAddr);
}

void HAL_PerfMGetAllCount(void)
{
    U8 ucCntIdx;
    for (ucCntIdx = 0; ucCntIdx < PM_COUNT_MAX; ucCntIdx++) 
    {
        g_aPerfMCnt[ucCntIdx] = HAL_PerfMGetCount(ucCntIdx);
    }
    return;
}

U32 HAL_PerfMSchedule(void)
{
    U8 ucCntIdx;
    LOCAL l_sCntState = SESS_SETUP;

    switch (l_sCntState)
    {
        case SESS_SETUP:
        {   
            //now only use count0
            U32 ulCntPreValue = g_bPerfMClrCnt ? 0 : INVALID_8F;
            HAL_PerfMSetUp(0, ulCntPreValue, CYCLE_CNT, NON_ZERO);

            HAL_PerfMStartCounting();
            l_sCntState = COUNTING;
        }break;

        case COUNTING:
        {   
            //check if all INTASRTs are clear
            for (ucCntIdx = 0; ucCntIdx < PM_COUNT_MAX; ucCntIdx++)
            {
                if (PM_INTASRT == HAL_PerfMGetInterruptType(ucCntIdx))
                {
                    l_sCntState = INT_TRIGD;
                    break;
                }
            }
            if (g_bPerfMRdCnt)//if no interrupt, but user need read count, also move to next state
            {
                l_sCntState = INT_TRIGD;
            }

        }break;

        case INT_TRIGD:
        {
            //clear PMEn
            HAL_PerfMDisableCounting();
            l_sCntState = DENOUEMENT;
        }break;

        case DENOUEMENT:
        { 
            //read counting
            HAL_PerfMGetAllCount();

            //clear all interrupt
            for (ucCntIdx = 0; ucCntIdx < PM_COUNT_MAX; ucCntIdx++)
            {
                //read counting
                g_aPerfMCnt[ucCntIdx] = HAL_PerfMGetCount(ucCntIdx);

                if (PM_INTASRT == HAL_PerfMGetInterruptType(ucCntIdx))
                {
                    HAL_PerfMHandleInterrupt(ucCntIdx, PM_INTASRT);
                    HAL_PerfMClrInterrupt(ucCntIdx, PM_INTASRT);
                }
            }
            
            l_sCntState = SESS_SETUP;
        }break;

        default:
        {
            DBG_Printf("PM not support this state!\n");
            DBG_Getch();
        }
    }
    return;
}

void TEST_PM(void)
{
    U8 ucIdx;
    DBG_Printf("PM Test Start\n");
    HAL_PerfMInit();

    /* all countes is used to cycle cnt */
    HAL_PerfMSetUp(0, INVALID_8F, CYCLE_CNT, NON_ZERO);
    for (ucIdx = 1; ucIdx < PM_COUNT_MAX; ucIdx++)
    {
        HAL_PerfMSetUp(1, INVALID_8F, OVERFLOW, NON_ZERO);
    }
    
    g_aPerfMCnt[0] = HAL_PerfMGetCount(0);
    DBG_Printf("Start of counting: g_aPerfMCnt = %d\n", g_aPerfMCnt[0]);

    /* enable and disable counting will cost 59 cycles */
    HAL_PerfMStartCounting();

    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");

    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");

    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");  //total 20 cycles
    
    HAL_PerfMDisableCounting();  
    g_aPerfMCnt[0] = HAL_PerfMGetCount(0);

    DBG_Printf("End of counting: g_aPerfMCnt = %d\n", g_aPerfMCnt[0]);   
}



#endif//#if (defined(FPGA) || defined(ASIC))

/* end of this file */
