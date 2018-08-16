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
Filename    : TEST_MCU0Mix.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "BaseDef.h"
#include "MixVector_SRAM.h"
#include "COM_Memory.h"
#include "COM_QWord.h"
#include "HAL_GLBReg.h"
#include "HAL_HCT.h"
#include "HAL_HSG.h"
#include "HAL_NormalDSG.h"
#include "HAL_SGE.h"
#include "HAL_Xtensa.h"

#define TESTMCU0_FCQ_LEN_BYTE      0x120

#define MCU0_DRQ_CNT               32 //need to confirm
#define MCU0_DWQ_CNT               64 //need to confirm
#define TESTMCU0_DWQ_SLOT_CNT      8
#define TESTMCU0_DRQ_SLOT_CNT      8
#define TESTMCU0_DWQ_SLOT_MIN      0
#define TESTMCU0_DWQ_SLOT_MAX      (TESTMCU0_DWQ_SLOT_CNT - 1)
#define TESTMCU0_DRQ_SLOT_MIN      TESTMCU0_DWQ_SLOT_CNT
#define TESTMCU0_DRQ_SLOT_MAX      (TESTMCU0_DRQ_SLOT_MIN + TESTMCU0_DRQ_SLOT_CNT - 1)
#define TESTMCU0_DWQ_SLOT_CHAIN    (MCU0_DWQ_CNT / TESTMCU0_DWQ_SLOT_CNT) //4
#define TESTMCU0_DRQ_SLOT_CHAIN    (MCU0_DRQ_CNT / TESTMCU0_DRQ_SLOT_CNT) //2
#define TESTMCU0_SGE_LEN_BYTE      (32 * 1024) //32K
#define TESTMCU0_DWQ_BUF_CNT       32
#define TESTMCU0_DRQ_BUF_CNT       32

typedef struct _TEST_SGE_INFO
{
    U16 usHsgID;
    U16 usDsgID;
    U16 usBufptr;
    U8  ucCurHID;
    U8  ucCurChainNum;
}TEST_SGE_INFO;

LOCAL volatile HCT_FCQ_REG *l_pFcqReg;
LOCAL QWORD l_qwFcqHostBase;

LOCAL U32 l_ulDwqDramBase;
LOCAL QWORD l_qwDwqHostBase;
LOCAL U32 l_ulDrqDramBase;
LOCAL QWORD l_qwDrqHostBase;
LOCAL TEST_SGE_INFO l_tDwqInfo;
LOCAL TEST_SGE_INFO l_tDrqInfo;
LOCAL

void TEST_MCU0FcqInit(void)
{
    HCT_INIT_PARAM tHCTInitParam;

    tHCTInitParam.tFCQParam.ulBaseAddr = HCT_FCQ_BASE;

    tHCTInitParam.tWBQParam.ulBaseAddr = HCT_WBQ_BASE;
    tHCTInitParam.tWBQParam.usTriggerState = 7;

    HAL_HCTInit(&tHCTInitParam);

    l_pFcqReg = &rHCT_FCQ_REG;

    l_qwFcqHostBase.LowDw = rP0_CLB;
    l_qwFcqHostBase.HighDw = rP0_CLBU;

    DBG_Printf("FCQ host addr: 0x%x%x\n", l_qwFcqHostBase.HighDw, l_qwFcqHostBase.LowDw);

    return;
}

void TEST_MCU0SgeInit(void)
{
    HAL_HsgInit();
    HAL_NormalDsgInit();
    HAL_DrqInit();
    HAL_DwqInit();

    l_ulDwqDramBase = DRAM_DATA_BUFF_MCU0_BASE;
    COM_QwAddDw(&l_qwFcqHostBase, 1024 * 1024, &l_qwDwqHostBase); //give FCQ test 1M
    DBG_Printf("DWQ host addr: 0x%x%x\n", l_qwDwqHostBase.HighDw, l_qwDwqHostBase.LowDw);
    l_ulDrqDramBase = DRAM_DATA_BUFF_MCU0_BASE + TESTMCU0_SGE_LEN_BYTE * TESTMCU0_DWQ_BUF_CNT;
    COM_QwAddDw(&l_qwDwqHostBase, MCU0_DWQ_CNT * TESTMCU0_SGE_LEN_BYTE, &l_qwDrqHostBase); //give DRQ test 4M
    DBG_Printf("DRQ host addr: 0x%x%x\n", l_qwDrqHostBase.HighDw, l_qwDrqHostBase.LowDw);

    l_tDwqInfo.usDsgID = INVALID_4F;
    l_tDwqInfo.usHsgID = INVALID_4F;
    l_tDwqInfo.usBufptr = 0;
    l_tDwqInfo.ucCurHID = TESTMCU0_DWQ_SLOT_MIN;
    l_tDwqInfo.ucCurChainNum = 0;
    l_tDrqInfo.usDsgID = INVALID_4F;
    l_tDrqInfo.usHsgID = INVALID_4F;
    l_tDrqInfo.usBufptr = 0;
    l_tDrqInfo.ucCurHID = TESTMCU0_DRQ_SLOT_MIN;
    l_tDrqInfo.ucCurChainNum = 0;

    return;
}

void TEST_MCU0FcqJustBuild(void)
{
    volatile HCT_FCQ_WBQ *pFCQEntry;
    U8 ucFcqID;
    QWORD qwHostAddr;

    pFCQEntry = HAL_HCTGetFCQEntry();

    if (NULL == pFCQEntry)
    {
        return;
    }

    COM_MemZero((U32 *)pFCQEntry, sizeof(HCT_FCQ_WBQ)/sizeof(U32));
    ucFcqID = l_pFcqReg->bsFCQWP;

    pFCQEntry->bsOffset = TESTMCU0_FCQ_LEN_BYTE * ucFcqID;
    pFCQEntry->bsLength = TESTMCU0_FCQ_LEN_BYTE / sizeof(U32);

    COM_QwAddDw(&l_qwFcqHostBase, pFCQEntry->bsOffset, &qwHostAddr);
    pFCQEntry->ulHostAddrLow = qwHostAddr.LowDw;
    pFCQEntry->ulHostAddrHigh = qwHostAddr.HighDw;

    HAL_HCTPushFCQEntry();

    DBG_Printf("build FCQ 0x%x\n", ucFcqID);

    return;
}

U16 TEST_MCU0DsgBuild(U32 ulDramAddr)
{
    NORMAL_DSG_ENTRY *pCurDSG;
    U16 usCurDSGId;

    if (FALSE == HAL_GetCurNormalDsg(&usCurDSGId))
    {
        return INVALID_4F;
    }

    pCurDSG = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(usCurDSGId);
    HAL_TriggerNormalDsg();
    COM_MemZero((U32 *)pCurDSG, sizeof(NORMAL_DSG_ENTRY) / sizeof(U32));

    pCurDSG->bsDramAddr = ulDramAddr;
    pCurDSG->bsXferByteLen = TESTMCU0_SGE_LEN_BYTE;
    pCurDSG->bsLast = TRUE;

    HAL_SetNormalDsgSts(usCurDSGId, 1);

    return usCurDSGId;
}

U16 TEST_MCU0HsgBuild(QWORD *pHostAddr)
{
    U16 usCurHSGId;
    HSG_ENTRY *pCurHSG;

    if (FALSE == HAL_GetCurHsg(&usCurHSGId))
    {
        return INVALID_4F;
    }

    pCurHSG = (HSG_ENTRY *)HAL_GetHsgAddr(usCurHSGId);
    HAL_TriggerHsg();
    COM_MemZero((U32 *)pCurHSG, sizeof(HSG_ENTRY) / sizeof(U32));

    pCurHSG->ulHostAddrLow = pHostAddr->LowDw;
    pCurHSG->ulHostAddrHigh = pHostAddr->HighDw;
    pCurHSG->bsLength = TESTMCU0_SGE_LEN_BYTE;
    pCurHSG->bsLast = TRUE;

    HAL_SetHsgSts(usCurHSGId, 1);

    return usCurHSGId;
}

BOOL TEST_MCU0DwqJustBuild(void)
{
    U32 ulDramAddr;
    QWORD qwHostAddr;

    if (INVALID_4F == l_tDwqInfo.usDsgID)
    {
        ulDramAddr = l_ulDwqDramBase + l_tDwqInfo.usBufptr * TESTMCU0_SGE_LEN_BYTE;

        l_tDwqInfo.usDsgID = TEST_MCU0DsgBuild(ulDramAddr);
    }

    if (INVALID_4F == l_tDwqInfo.usHsgID)
    {
        COM_QwAddDw(&l_qwDwqHostBase, l_tDwqInfo.usBufptr * TESTMCU0_SGE_LEN_BYTE, &qwHostAddr);

        l_tDwqInfo.usHsgID = TEST_MCU0HsgBuild(&qwHostAddr);
    }

    if ((INVALID_4F != l_tDwqInfo.usDsgID) && (INVALID_4F != l_tDwqInfo.usHsgID))
    {
        if (FALSE == HAL_DwqBuildEntry(l_tDwqInfo.ucCurHID, l_tDwqInfo.usHsgID, l_tDwqInfo.usDsgID))
        {
            return FALSE;
        }
        else
        {
            DBG_Printf("Build DWQ: HsgID 0x%x, DsgID 0x%x\n", l_tDwqInfo.usHsgID, l_tDwqInfo.usDsgID);
            l_tDwqInfo.usDsgID = INVALID_4F;
            l_tDwqInfo.usHsgID = INVALID_4F;

            l_tDwqInfo.usBufptr++;
            if (TESTMCU0_DWQ_BUF_CNT == l_tDwqInfo.usBufptr)
            {
                l_tDwqInfo.usBufptr = 0;
            }

            l_tDwqInfo.ucCurChainNum++;
            if (TESTMCU0_DWQ_SLOT_CHAIN == l_tDwqInfo.ucCurChainNum)
            {
                //HAL_SgeHelpFinishChainCnt(l_tDwqInfo.ucCurHID);
                //HAL_SgeFinishChainCnt(l_tDwqInfo.ucCurHID, l_tDwqInfo.ucCurChainNum);

                l_tDwqInfo.ucCurHID++;
                if (TESTMCU0_DWQ_SLOT_MAX == l_tDwqInfo.ucCurHID)
                {
                    l_tDwqInfo.ucCurHID = TESTMCU0_DWQ_SLOT_MIN;
                }

                l_tDwqInfo.ucCurChainNum = 0;
            }

            return TRUE;
        }
    } //if ((INVALID_4F != l_tDwqInfo.usDsgID) && (INVALID_4F != l_tDwqInfo.usHsgID))

    return FALSE;
}

BOOL TEST_MCU0DrqJustBuild(void)
{
    U32 ulDramAddr;
    QWORD qwHostAddr;

    if (INVALID_4F == l_tDrqInfo.usDsgID)
    {
        ulDramAddr = l_ulDrqDramBase + l_tDrqInfo.usBufptr * TESTMCU0_SGE_LEN_BYTE;

        l_tDrqInfo.usDsgID = TEST_MCU0DsgBuild(ulDramAddr);
    }

    if (INVALID_4F == l_tDrqInfo.usHsgID)
    {
        COM_QwAddDw(&l_qwDrqHostBase, l_tDrqInfo.usBufptr * TESTMCU0_SGE_LEN_BYTE, &qwHostAddr);

        l_tDrqInfo.usHsgID = TEST_MCU0HsgBuild(&qwHostAddr);
    }

    if ((INVALID_4F != l_tDrqInfo.usDsgID) && (INVALID_4F != l_tDrqInfo.usHsgID))
    {
        if (FALSE == HAL_DrqBuildEntry(l_tDrqInfo.ucCurHID, l_tDrqInfo.usHsgID, l_tDrqInfo.usDsgID))
        {
            return FALSE;
        }
        else
        {
            DBG_Printf("Build DRQ: HsgID 0x%x, DsgID 0x%x\n", l_tDrqInfo.usHsgID, l_tDrqInfo.usDsgID);
            l_tDrqInfo.usDsgID = INVALID_4F;
            l_tDrqInfo.usHsgID = INVALID_4F;

            l_tDrqInfo.usBufptr++;
            if (TESTMCU0_DRQ_BUF_CNT == l_tDrqInfo.usBufptr)
            {
                l_tDrqInfo.usBufptr = 0;
            }

            l_tDrqInfo.ucCurChainNum++;
            if (TESTMCU0_DRQ_SLOT_CHAIN == l_tDrqInfo.ucCurChainNum)
            {
                //HAL_SgeFinishChainCnt(l_tDrqInfo.ucCurHID, l_tDrqInfo.ucCurChainNum);

                l_tDrqInfo.ucCurHID++;
                if (TESTMCU0_DRQ_SLOT_MAX == l_tDrqInfo.ucCurHID)
                {
                    l_tDrqInfo.ucCurHID = TESTMCU0_DRQ_SLOT_MIN;
                }

                l_tDrqInfo.ucCurChainNum = 0;
            }

            return TRUE;
        }
    } //if ((INVALID_4F != l_tDwqInfo.usDsgID) && (INVALID_4F != l_tDwqInfo.usHsgID))

    return FALSE;
}

void TEST_MCU0SetMutiCoreMode(void)
{
    rGlbMCUSramMap = 0x0;
    return;
}

void TEST_MCU0SetMCU12IsramSZ(void)
{
    rGLB(0x3C) |= 0x4;
    return;
}

void TEST_MCU0BootMcu12FromIsram(void)
{
    //configure mcu1/2 boot from isram
    rGlbMCUCtrl = 0x0;
    rGlbMCUCtrl |= 0x110;

    //enable mcu1/2 boot up
    rGlbMcuSgeRst = 0x0;
}

void TEST_MCU0MixMain(void)
{
#if defined(COSIM) //only COSIM ENV need FW code to initialize DDR
    HAL_DramcInit();

    TEST_MCU0SetMutiCoreMode();
    TEST_MCU0SetMCU12IsramSZ();
    TEST_MCU0BootMcu12FromIsram();
#endif

    TEST_MCU0FcqInit();
    TEST_MCU0SgeInit();
    DBG_Printf("TEST_MCU0MixMain: init done\n");

    while (1)
    {
        TEST_MCU0FcqJustBuild();

        TEST_MCU0DwqJustBuild();

        TEST_MCU0DrqJustBuild();
    }
}

