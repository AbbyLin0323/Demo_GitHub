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

#include "BaseDef.h"
#include "COM_Memory.h"
#ifdef SIM
#include "windows.h"
#include "Sim_HCT.h"
#endif
#include "HAL_NVME.h"
#include "L0_NVMe.h"
#include "L0_NVMeHCT.h"
#include "L0_Interface.h"
#include "HAL_MultiCore.h"

volatile HCT_CONTROL_REG    *g_pHCTControlReg;

void L0_NVMEInitHCT(void)
{
    g_pHCTControlReg = (volatile HCT_CONTROL_REG*)(REG_BASE_HCT + 0x14);

    HCT_INIT_PARAM tHCTInitParam;

    tHCTInitParam.tFCQParam.ulBaseAddr = HCT_FCQ_BASE;

    tHCTInitParam.tWBQParam.ulBaseAddr = HCT_WBQ_BASE;
    tHCTInitParam.tWBQParam.usTriggerState = (U16)SLOT_TRIGGER_WBQ;
    tHCTInitParam.tWBQParam.usIncrement = sizeof(HCT_FCQ_WBQ) * WBQ_N;

    COM_MemZero((U32 *)&tHCTInitParam.aSRAMTableParam[0],
        sizeof(tHCTInitParam.aSRAMTableParam)/sizeof(U32));

    tHCTInitParam.aSRAMTableParam[0].ulBaseAddr = HCT_S0_BASE;
    tHCTInitParam.aSRAMTableParam[0].ulIncrement = sizeof(COMMAND_HEADER);
    tHCTInitParam.aSRAMTableParam[1].ulBaseAddr = HCT_S1_BASE;
    tHCTInitParam.aSRAMTableParam[1].ulIncrement = sizeof(COMMAND_TABLE);


    HAL_HCTInit(&tHCTInitParam);

    g_pHCTControlReg->bCQFullChkPlcy = FALSE;

    return;
}

#ifndef AF_ENABLE
U32 L0_NVMeHCTReadCH( PCB_MGR CbMgr )
{
    volatile HCT_FCQ_WBQ *pFCQEntry;
    U32 ulRet;
    U8  ucSQID;


    /* Attempts to get a FCQ entry. */
    pFCQEntry = HAL_HCTGetFCQEntry();

    /* Checking whether FCQ is full currently. */
    if (NULL == pFCQEntry)
    {
        ulRet = FAIL;
    }
    else
    {
        /* Programs and issues FCQ entry to hardware if FCQ is not full. */
        ulRet = SUCCESS;
        ucSQID = CbMgr->SQID;

        HAL_HCTSetCST(CbMgr->SlotNum, SLOT_WAITING_SQENTRY);

        COM_MemZero((U32 *)pFCQEntry, sizeof(HCT_FCQ_WBQ)/sizeof(U32));
        pFCQEntry->bsID = CbMgr->SlotNum;
        pFCQEntry->bsIDB = TRUE;
        pFCQEntry->bsSN = SN_COMMAND_HEADER;
        pFCQEntry->bsNST = SLOT_SQENTRY_RDY;
        pFCQEntry->bsCST = SLOT_WAITING_SQENTRY;
        pFCQEntry->bsLength = sizeof( COMMAND_HEADER );
        pFCQEntry->bsUpdate = TRUE;
        pFCQEntry->ulHostAddrLow = SQ_HOST_BAL(ucSQID)+ SQ_FWRP(ucSQID)* sizeof(COMMAND_HEADER);
        if(pFCQEntry->ulHostAddrLow < SQ_HOST_BAL(ucSQID))
        {
            pFCQEntry->ulHostAddrHigh = SQ_HOST_BAH(ucSQID) + 1;
        }
        else
        {
            pFCQEntry->ulHostAddrHigh = SQ_HOST_BAH(ucSQID);
        }

        HAL_HCTPushFCQEntry();
    }

    return ulRet;
}
#endif

U32 L0_NVMeHCTReadPRP(PCB_MGR CbMgr)
{
    U32 ulRet;
    volatile HCT_FCQ_WBQ *pFCQEntry;


    /* Attempts to get a FCQ entry. */
    pFCQEntry = HAL_HCTGetFCQEntry();

    /* Checking whether FCQ is full currently. */
    if (NULL == pFCQEntry)
    {
        ulRet = FAIL;
    }
    else
    {
        /* Programs and issues FCQ entry to hardware if FCQ is not full. */
        ulRet = SUCCESS; 

        HAL_HCTSetCST(CbMgr->SlotNum, SLOT_WAITING_PRPLIST);
        COM_MemZero((U32 *)pFCQEntry, sizeof(HCT_FCQ_WBQ)/sizeof(U32));
        pFCQEntry->bsIDB = FALSE;
        pFCQEntry->bsOffset = (U32)(&CbMgr->PRPTable[1]) - DSRAM1_MCU012_SHARE_BASE;
        pFCQEntry->bsID = CbMgr->SlotNum;
        pFCQEntry->ulHostAddrHigh = CbMgr->Ch->PRP2H;
        pFCQEntry->ulHostAddrLow = CbMgr->Ch->PRP2L;
        pFCQEntry->bsNST = SLOT_PRPLIST_RDY;
        pFCQEntry->bsCST = SLOT_WAITING_PRPLIST;
        pFCQEntry->bsLength = sizeof(PRP) * (CbMgr->TotalPRPCnt - 1);
        pFCQEntry->bsUpdate = TRUE;

        HAL_HCTPushFCQEntry();
    }

    return ulRet;
}



