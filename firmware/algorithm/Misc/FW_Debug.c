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
Filename    :FW_Debug.c
Version     :Ver 1.0
Author      :AwayWei
Date        :2013.04.03
Description :Main function for COM Debug
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "FW_Debug.h"
#ifdef SIM
#include "sim_flash_common.h"
#endif

#include "Disk_Config.h"
#include "L1_GlobalInfo.h"
#include "L1_Ramdisk.h"
#include "FW_BufAddr.h"
#include "HAL_TraceLog.h"
#include "COM_Memory.h"
#include "HAL_Inc.h"
#include "HAL_Xtensa.h"
#include "HAL_TraceLog.h"
#include "HAL_IIC.h"
#include "HAL_TSC.h"
#include "HAL_TemperatureSensor.h"
#ifndef L1_FAKE
#include "L3_Interface.h"
#include "L3_Schedule.h"
#include "L2_FTL.h"
#include "L2_RT.h"
#include "L2_FCMDQ.h"
#include "L2_Interface.h"
#include "L3_Debug.h"
#endif

//specify file name for Trace Log
#define TL_FILE_NUM  FW_Debug_c

TRACE_BLOCK_MANAGEMENT g_TraceManagement;

extern GLOBAL U32 g_L2TempBufferAddr;
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
extern IIC_SENSOR_PARAM g_IICSensorParam;
extern TSC_SENSOR_PARAM g_TSCSensorParam;

extern void L1_DbgShowAll(void);
extern void L2_DbgShowAll(void);
extern void L3_DbgShowAll(void);
extern void FW_DbgShowAll(void);
extern BOOL HAL_SgeGetAllEngIdle(void);
extern BOOL HAL_SgeGetAllTransFsh(void);
#ifndef SIM
volatile U32 g_dbg_enable;

void MCU12_DRAM_TEXT DBG_Getch_Init(void)
{
    g_dbg_enable = 1;
    return;
}

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
    U32 ulMcuId = HAL_GetMcuId();

    DBG_Printf("Fatal Error, MCU %d DBG_Getch!!!\n", ulMcuId);  while(1);

    FW_DbgShowAll();

#ifndef L1_FAKE
    //get a Idle slot and send SCMD to subsystem, dirtectly save subsystem trace
        FW_FlushSubsystemTrace(NULL);

    /* send DBG_Getch inform SMSG to MCU0 and enable L1 Ramdisk */
    //L1_RamdiskSetMode(L1_RAMDISK_ENABLE_SPECIAL);
    //L1_RamdiskReportGetchSMSG();
#endif

    DBG_Getch_Init();

    while(g_dbg_enable)
    {
        /* dead loop in L1 Debug Ramdisk */
        //L1_RamdiskSchedule();
    }

}
#endif

#ifndef HOST_SATA
/****************************************************************************
Name        :HAL_SGEAllEngStatus
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_SGEAllEngStatus(void)
{
    BOOL ulAllTransFinish,ulAllEngIdle;

    ulAllEngIdle = HAL_SgeGetAllEngIdle();
    ulAllTransFinish = HAL_SgeGetAllTransFsh();

    TRACE_LOG((void*)&ulAllEngIdle, sizeof(BOOL), BOOL, 0, "[SGE] is AllEngIdle:");
    TRACE_LOG((void*)&ulAllTransFinish, sizeof(BOOL), BOOL, 0, "[SGE] is AllTransFinish:");
    return;
}

/****************************************************************************
Name        :HAL_DRQDWQStatus
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_DRQDWQStatus(void)
{

    volatile DRQ_DWQ_REG tQStaus;

     tQStaus.ulValue = rDrqMcu0Status;
    TRACE_LOG((void*)&tQStaus, sizeof(DRQ_DWQ_REG), DRQ_DWQ_REG, 0, "[SGE] DrqMcu0:");

    tQStaus.ulValue = rDwqMcu0Status;
    TRACE_LOG((void*)&tQStaus, sizeof(DRQ_DWQ_REG), DRQ_DWQ_REG, 0, "[SGE] DwqMcu0:");

    tQStaus.ulValue = rDrqMcu1Status;
    TRACE_LOG((void*)&tQStaus, sizeof(DRQ_DWQ_REG), DRQ_DWQ_REG, 0, "[SGE] DrqMcu1:");

    tQStaus.ulValue = rDwqMcu1Status;
    TRACE_LOG((void*)&tQStaus, sizeof(DRQ_DWQ_REG), DRQ_DWQ_REG, 0, "[SGE] DwqMcu1:");

    tQStaus.ulValue = rDrqMcu2Status;
    TRACE_LOG((void*)&tQStaus, sizeof(DRQ_DWQ_REG), DRQ_DWQ_REG, 0, "[SGE] DrqMcu2:");

    tQStaus.ulValue = rDwqMcu2Status;
    TRACE_LOG((void*)&tQStaus, sizeof(DRQ_DWQ_REG), DRQ_DWQ_REG, 0, "[SGE] DwqMcu2:");

    return;
}


/****************************************************************************
Name        :HAL_SgeOtfbActiveBitmap
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_SgeOtfbActiveBitmap(void)
{
    SGE_OTFB_TIG_STATUS tOtfbTigStatus;
    U32 ulDebugValue,ulCmdSlotGroup;

    ulDebugValue  = SGE_DEBUG_OTFB_ACTIVE_BITMAP;
    for(ulCmdSlotGroup = 0; ulCmdSlotGroup < 7;ulCmdSlotGroup ++)
    {
        TRACE_LOG((void*)&ulCmdSlotGroup, sizeof(U32), U32, 0, "[SGE] OtfbActive Group:");

        rSgeDebugInput = (ulDebugValue + ulCmdSlotGroup);

        tOtfbTigStatus.ulValue = rSgeDebugOutput0;
        TRACE_LOG((void*)&tOtfbTigStatus, sizeof(SGE_OTFB_TIG_STATUS), SGE_OTFB_TIG_STATUS, 0, "[SGE] ActiveBitmap0:");

        tOtfbTigStatus.ulValue = rSgeDebugOutput1;
        TRACE_LOG((void*)&tOtfbTigStatus, sizeof(SGE_OTFB_TIG_STATUS), SGE_OTFB_TIG_STATUS, 0, "[SGE] ActiveBitmap1:");

        tOtfbTigStatus.ulValue = rSgeDebugOutput2;
        TRACE_LOG((void*)&tOtfbTigStatus, sizeof(SGE_OTFB_TIG_STATUS), SGE_OTFB_TIG_STATUS, 0, "[SGE] ActiveBitmap2:");

        tOtfbTigStatus.ulValue = rSgeDebugOutput3;
        TRACE_LOG((void*)&tOtfbTigStatus, sizeof(SGE_OTFB_TIG_STATUS), SGE_OTFB_TIG_STATUS, 0, "[SGE] ActiveBitmap3:");
    }
    return;
}


/****************************************************************************
Name        :HAL_SgeChainCntStatus
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_SgeChainCntStatus(void)
{
    SGE_HID_CHAIN_NUM tChainCntStatus;
    U32 ulDebugValue,ulCmdSlotGroup;

    ulDebugValue  = SGE_DEBUG_HID_CHAIN_NUM;
    for(ulCmdSlotGroup = 0; ulCmdSlotGroup < (MAX_SLOT_NUM / 4);ulCmdSlotGroup ++)
    {
        TRACE_LOG((void*)&ulCmdSlotGroup, sizeof(U32), U32, 0, "[SGE] ChainCntStatus Group:");

        rSgeDebugInput = (ulDebugValue + ulCmdSlotGroup);

        tChainCntStatus.ulValue = rSgeDebugOutput0;
        TRACE_LOG((void*)&tChainCntStatus, sizeof(SGE_HID_CHAIN_NUM), SGE_HID_CHAIN_NUM, 0, "[SGE] ChainCntStatus0:");

        tChainCntStatus.ulValue = rSgeDebugOutput1;
        TRACE_LOG((void*)&tChainCntStatus, sizeof(SGE_HID_CHAIN_NUM), SGE_HID_CHAIN_NUM, 0, "[SGE] ChainCntStatus1:");

        tChainCntStatus.ulValue = rSgeDebugOutput2;
        TRACE_LOG((void*)&tChainCntStatus, sizeof(SGE_HID_CHAIN_NUM), SGE_HID_CHAIN_NUM, 0, "[SGE] ChainCntStatus2:");

        tChainCntStatus.ulValue = rSgeDebugOutput3;
        TRACE_LOG((void*)&tChainCntStatus, sizeof(SGE_HID_CHAIN_NUM), SGE_HID_CHAIN_NUM, 0, "[SGE] ChainCntStatus3:");
    }
    return;
}

/****************************************************************************
Name        :HAL_SgeOtfbTxOnGoing
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_SgeOtfbTxOnGoing(void)
{
    SGE_OTFB_TRANS_ENG tOtfbTransStatus;
    U32 ulDebugValue,ulCmdSlotGroup;

    ulDebugValue  = SGE_DEBUG_OTFB_TRANS;
    for(ulCmdSlotGroup = 0; ulCmdSlotGroup < 7;ulCmdSlotGroup ++)
    {
        TRACE_LOG((void*)&ulCmdSlotGroup, sizeof(U32), U32, 0, "[SGE] OTFB TxOnGoing Group:");

        rSgeDebugInput = (ulDebugValue + ulCmdSlotGroup);

        tOtfbTransStatus.ulValue = rSgeDebugOutput0;
        TRACE_LOG((void*)&tOtfbTransStatus, sizeof(SGE_OTFB_TRANS_ENG), SGE_OTFB_TRANS_ENG, 0, "[SGE] TxOnGoing0:");

        tOtfbTransStatus.ulValue = rSgeDebugOutput1;
        TRACE_LOG((void*)&tOtfbTransStatus, sizeof(SGE_OTFB_TRANS_ENG), SGE_OTFB_TRANS_ENG, 0, "[SGE] TxOnGoing1:");

        tOtfbTransStatus.ulValue = rSgeDebugOutput2;
        TRACE_LOG((void*)&tOtfbTransStatus, sizeof(SGE_OTFB_TRANS_ENG), SGE_OTFB_TRANS_ENG, 0, "[SGE] TxOnGoing2:");

        tOtfbTransStatus.ulValue = rSgeDebugOutput3;
        TRACE_LOG((void*)&tOtfbTransStatus, sizeof(SGE_OTFB_TRANS_ENG), SGE_OTFB_TRANS_ENG, 0, "[SGE] TxOnGoing3:");
    }
    return;
}


/****************************************************************************
Name        :HAL_SgeDramTxOnGoing
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_SgeDramTxOnGoing(void)
{
    SGE_DRAM_TRANS_ENG tDramTransStatus;
    U32 ulDebugValue;

    ulDebugValue  = SGE_DEBUG_DRAM_TRANS;
    rSgeDebugInput = ulDebugValue;

    tDramTransStatus.ulValue = rSgeDebugOutput0;
    TRACE_LOG((void*)&tDramTransStatus, sizeof(SGE_DRAM_TRANS_ENG), SGE_DRAM_TRANS_ENG, 0, "[SGE] MCU1DR TxOnGoing:");

    tDramTransStatus.ulValue = rSgeDebugOutput1;
    TRACE_LOG((void*)&tDramTransStatus, sizeof(SGE_DRAM_TRANS_ENG), SGE_DRAM_TRANS_ENG, 0, "[SGE] MCU2DR TxOnGoing:");

    tDramTransStatus.ulValue = rSgeDebugOutput2;
    TRACE_LOG((void*)&tDramTransStatus, sizeof(SGE_DRAM_TRANS_ENG), SGE_DRAM_TRANS_ENG, 0, "[SGE] MCU1DW TxOnGoing:");

    tDramTransStatus.ulValue = rSgeDebugOutput3;
    TRACE_LOG((void*)&tDramTransStatus, sizeof(SGE_DRAM_TRANS_ENG), SGE_DRAM_TRANS_ENG, 0, "[SGE] MCU2DW TxOnGoing:");

    ulDebugValue  = (SGE_DEBUG_DRAM_TRANS + 1);
    rSgeDebugInput = ulDebugValue;

    tDramTransStatus.ulValue = rSgeDebugOutput0;
    TRACE_LOG((void*)&tDramTransStatus, sizeof(SGE_DRAM_TRANS_ENG), SGE_DRAM_TRANS_ENG, 0, "[SGE] MCU3DR TxOnGoing:");

    tDramTransStatus.ulValue = rSgeDebugOutput2;
    TRACE_LOG((void*)&tDramTransStatus, sizeof(SGE_DRAM_TRANS_ENG), SGE_DRAM_TRANS_ENG, 0, "[SGE] MCU3DW TxOnGoing:");

    return;
}

/****************************************************************************
Name        :HAL_SGEDbgShowAll
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_SGEDbgShowAll(void)
{

    HAL_SGEAllEngStatus();

    HAL_DRQDWQStatus();

    HAL_SgeOtfbActiveBitmap();

    HAL_SgeChainCntStatus();

    HAL_SgeOtfbTxOnGoing();

    HAL_SgeDramTxOnGoing();

    return;
}

#endif
/****************************************************************************
Name        :FW_HALDbgShowAll
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2015.04.02
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_DbgShowAll(void)
{

#ifndef HOST_SATA
    HAL_SGEDbgShowAll();
#endif

    return;
}

/****************************************************************************
Name        :FW_DbgShowAll
Input       :NULL
Output      :NULL
Author      :AwayWei
Date        :2013.04.03
Description :.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT FW_DbgShowAll(void)
{
    HAL_DbgShowAll();

#ifndef L1_FAKE
    //L1_DbgShowAll();

    //L2_DbgShowAll();

    //L3_DbgShowAll();
#endif

    return;
}

/****************************************************************************
Name        :FW_DbgUpdateStaticInfo
Input       :
Output      :
Author      :AwayWei
Date        :2013.04.03
Description :.
Others      :
Modify      :2015.03.11 JasonGuo modify, add the calc interfaces
****************************************************************************/
void MCU12_DRAM_TEXT FW_DbgUpdateStaticInfo(void)
{
#if (!defined(L1_FAKE) && !defined(L2_FAKE))
    U32 ulTSC = 0;
    //get Erase Cnt form L2 instead of L3
    //g_pSubSystemDevParamPage->TotalEraseCount = L3_FMGetTotErsCnt();
    g_pSubSystemDevParamPage->TotalEraseCount = SUBSYSTEM_SUPERPU_NUM * pPBIT[0]->m_EraseCnt[BLKTYPE_TLC];
    
    g_pSubSystemDevParamPage->AvgEraseCount   = g_pSubSystemDevParamPage->TotalEraseCount/SUBSYSTEM_PU_NUM;
    g_pSubSystemDevParamPage->TotalNANDWrites = L3_FMGetTotPrgCnt();
    if ((g_IICSensorParam.ucDeviceType != NOSENSOR) || (g_TSCSensorParam.ucDeviceType != NOSENSOR))
    {
        if(MCU1_ID == HAL_GetMcuId())
        {
             if (g_IICSensorParam.ucDeviceType & I2C_SENSOR )
            {
                 g_pSubSystemDevParamPage->SYSTemperature = (U32)HAL_GetTemperature();
            }
            if (g_TSCSensorParam.ucDeviceType & TSC_SENSOR)
            {
                 ulTSC = (U32)HAL_TSCTempValueGet(1) /100;
                 if (ulTSC < g_TSCSensorParam.ucLoThreshold)
                 {
                     g_pSubSystemDevParamPage->SYSTemperature = ulTSC;
                 }
                 else if (ulTSC >= g_TSCSensorParam.ucLoThreshold && ulTSC <= g_TSCSensorParam.ucHiThreshold)
                 {
                     g_pSubSystemDevParamPage->SYSTemperature = g_TSCSensorParam.ucLoThreshold;
                 }
                 else if (ulTSC > g_TSCSensorParam.ucHiThreshold)
                 {
                     g_pSubSystemDevParamPage->SYSTemperature = ulTSC - g_TSCSensorParam.ucDelta;
                 }
            }
        }
        else
        {
            g_pSubSystemDevParamPage->SYSTemperature = 0;
        }
    }
    else
    {
        g_pSubSystemDevParamPage->SYSTemperature = 50;
    }
#endif

    if(g_pSubSystemDevParamPage->SYSTemperature > g_pSubSystemDevParamPage->WorstTemperature)
    {
       g_pSubSystemDevParamPage->WorstTemperature = g_pSubSystemDevParamPage->SYSTemperature;
    }

    return;
}

LOCAL void MCU12_DRAM_TEXT FW_TraceManagementInit()
{
    g_TraceManagement.m_TraceStage = TRACE_PREPARE;
}

void MCU12_DRAM_TEXT FW_TraceManagementLLF()
{
    g_TraceManagement.m_TracePhyPos = 0;
    g_TraceManagement.m_TraceEraseBlockSN = 0;
    g_TraceManagement.m_TracePhyPageRemain = TRACE_BLOCK_COUNT * SUBSYSTEM_LUN_NUM * (LOGIC_PG_PER_BLK);
    g_TraceManagement.m_TraceStage = TRACE_PREPARE;
    g_TraceManagement.m_NeedRebuild = FALSE;
    g_TraceManagement.m_TimeStamp = 0;
}

#ifndef L1_FAKE
/*****************************************************************************
 Prototype      : FW_TraceBlockRebuild
 Description    : recovery trace block
 Input          : void
 Output         : None
 Return Value   : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2013/9/23
   Author       : henryluo
   Modification : Created function

*****************************************************************************/
LOCAL void MCU12_DRAM_TEXT FW_TraceBlockRebuild(U32 CriticalFlag)
{
#if 0
    U8 ucPu;
    U16 usBlock;
    U32 ulStatus;
    FLASH_ADDR FlashPhyAddr = {0};

    for (usBlock = 0; usBlock < TRACE_BLOCK_COUNT; usBlock++)
    {
        for (ucPu = 0 ; ucPu < SUBSYSTEM_LUN_NUM ; ucPu++)
        {
            if(CriticalFlag)
            {
                FlashPhyAddr.ucPU = ucPu;
                FlashPhyAddr.usBlock = pRT->m_RT[ucPu].TraceBlockAddr[0][usBlock];
                FlashPhyAddr.usPage = 0;

                /* wait nfcq idle */
                while(TRUE != HAL_NfcGetIdle(ucPu, 0));

                if(NFC_STATUS_SUCCESS != HAL_NfcFullBlockErase(&FlashPhyAddr, FALSE))
                {
                    DBG_Getch();
                }

                /* wait write command done */
                ulStatus = HAL_NfcWaitStatus(FlashPhyAddr.ucPU, 0);
                if(NFC_STATUS_SUCCESS != ulStatus)
                {
                    DBG_Printf("MCU#%d FW_TraceBlockRebuild Getch\n", HAL_GetMcuId());
                    DBG_Getch();
                }
            }
            else
            {
                /* wait L3 CE FIFO empty */
                while(FALSE == L2_FCMDQNotFull(ucPu))
                {
                    //L3_Scheduler();

                    #ifdef SIM
                    //NFC_ModelSchedule();
                    #endif
                }

                /* if erase trace block fail, need add error handling here */
                L2_FtlEraseBlock(ucPu,0, pRT->m_RT[ucPu].TraceBlockAddr[0][usBlock], NULL, TRUE, TRUE);
            }
        }
    }

    FW_TraceManagementLLF();
#endif
}

/*****************************************************************************
 Prototype      : FW_TraceBlockErase
 Description    : erase trace block
 Input          : void
 Output         : None
 Return Value   : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2013/9/13
   Author       : henryluo
   Modification : Created function

*****************************************************************************/
LOCAL void MCU12_DRAM_TEXT FW_TraceBlockErase(U32 CriticalFlag)
{
#if 0
    U8 ucPu;
    U16 usBlock;
    U16 ucCurBlockSN;
    U32 ulStatus;
    FLASH_ADDR FlashPhyAddr = {0};

    usBlock = g_TraceManagement.m_TraceEraseBlockSN;
    ucCurBlockSN = g_TraceManagement.m_TracePhyPos / (SUBSYSTEM_LUN_NUM * (PG_PER_BLK/2));
    if(usBlock != ucCurBlockSN)
    {
        DBG_Getch();
    }

    for ( ucPu = 0 ; ucPu < SUBSYSTEM_LUN_NUM ; ucPu++ )
    {
        if(CriticalFlag)
        {
            FlashPhyAddr.ucPU = ucPu;
            FlashPhyAddr.usBlock = pRT->m_RT[ucPu].TraceBlockAddr[0][usBlock];
            FlashPhyAddr.usPage = 0;

            /* wait nfcq idle */
            while(TRUE != HAL_NfcGetIdle(ucPu, 0));

            if(NFC_STATUS_SUCCESS != HAL_NfcFullBlockErase(&FlashPhyAddr, FALSE))
            {
                DBG_Getch();
            }

            /* wait write command done */
            ulStatus = HAL_NfcWaitStatus(FlashPhyAddr.ucPU, 0);
            if(NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Getch();
            }
        }
        else
        {
            /* wait L3 CE FIFO empty */
            while(FALSE == L2_FCMDQNotFull(ucPu))
            {
                //L3_Scheduler();

                #ifdef SIM
                //NFC_ModelSchedule();
                #endif
            }

            L2_FtlEraseBlock(ucPu,0, pRT->m_RT[ucPu].TraceBlockAddr[0][usBlock], NULL, TRUE, TRUE);
        }
    }

    g_TraceManagement.m_TraceEraseBlockSN++;
    if(g_TraceManagement.m_TraceEraseBlockSN >= TRACE_BLOCK_COUNT)
    {
        g_TraceManagement.m_TraceEraseBlockSN = 0;
    }

    g_TraceManagement.m_TracePhyPageRemain += (SUBSYSTEM_LUN_NUM * (PG_PER_BLK/2));
#endif

}

/*****************************************************************************
 Prototype      : FW_TraceBlockSave
 Description    : save trace block
 Input          : void
 Output         : None
 Return Value : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2013/9/11
   Author       : henryluo
   Modification : Created function

*****************************************************************************/
LOCAL void MCU12_DRAM_TEXT FW_TraceBlockSave(U32 ulMcuId,U32 CriticalFlag,U32 ulSaveSize)
{
#if 0
    U32 ulBufferID;
    U32 ulTraceMemAddr = 0;
    PhysicalAddr FlashAddr = { 0 };
    FLASH_ADDR FlashPhyAddr = {0};
    RED Spare;
    U8 ucStatus;
    U32 ulBLockSN;
    U32 ulCurFlushSecCnt = 0;
    TL_INFO* pTlInfo;
    NFC_PRG_REQ_DES tDes = {0};

    TL_PrepareFlushData(ulMcuId,(U32*)&ulTraceMemAddr,(U8*)g_L2TempBufferAddr,&ulCurFlushSecCnt,ulSaveSize);

    pTlInfo = TL_GetTLInfo(ulMcuId);

    /* Considering save MCU0 trace in subsystem thread,to avoid read MCU0 TLInfo confilict when MCU0 is recording trace,
       so we decide to save all MCU0 memory trace once no matter how much it recorded.
       Host Tool need to distinguish latest trace buffer according to Spare information. */
    Spare.m_McuId = ulMcuId;
    Spare.m_TraceItemCNT = ulCurFlushSecCnt;
    Spare.m_TraceSN = g_TraceManagement.m_TimeStamp;
    COM_MemCpy((U32*)&Spare.m_TLInfo,(U32*)pTlInfo,sizeof(TL_INFO)/sizeof(U32));
    Spare.m_RedComm.bcPageType = PAGE_TYPE_TRACE;
    Spare.m_RedComm.bcBlockType = BLOCK_TYPE_TRACE;

    FlashAddr.m_PUSer = g_TraceManagement.m_TracePhyPos % SUBSYSTEM_LUN_NUM;
    FlashAddr.m_PageInBlock = g_TraceManagement.m_TracePhyPos / SUBSYSTEM_LUN_NUM;

    ulBLockSN = FlashAddr.m_PageInBlock / (PG_PER_BLK/2);

    FlashAddr.m_BlockInPU = pRT->m_RT[FlashAddr.m_PUSer].TraceBlockAddr[0][ulBLockSN];
    FlashAddr.m_PageInBlock = FlashAddr.m_PageInBlock % (PG_PER_BLK/2);
    FlashAddr.m_LPNInPage = 0;

    if(CriticalFlag)
    {
        FlashPhyAddr.ucPU = FlashAddr.m_PUSer;
        FlashPhyAddr.usBlock = FlashAddr.m_BlockInPU;
        FlashPhyAddr.usPage = FlashAddr.m_PageInBlock;

        /* wait nfcq idle */
        while(TRUE != HAL_NfcGetIdle(FlashPhyAddr.ucPU, 0));

        tDes.bsWrBuffId = COM_GetBufferIDByMemAddr(ulTraceMemAddr, TRUE, BUF_SIZE_BITS);
        tDes.bsRedOntf = TRUE;
        tDes.pNfcRed = (NFC_RED *)&Spare;
        tDes.pErrInj = NULL;

        if(NFC_STATUS_SUCCESS != HAL_NfcFullPageWrite(&FlashPhyAddr, &tDes))
        {
            DBG_Getch();
        }

        /* wait write command done */
        ucStatus = HAL_NfcWaitStatus(FlashPhyAddr.ucPU, FlashPhyAddr.ucLun);
        if(NFC_STATUS_SUCCESS != ucStatus)
        {
            DBG_Getch();
        }
    }
    else
    {
        /* wait L3 CE FIFO empty */
        while(FALSE == L2_FCMDQNotFull(FlashAddr.m_PUSer))
        {
            //L3_Scheduler();

            #ifdef SIM
            //NFC_ModelSchedule();
            #endif
        }

        L2_FtlWriteLocal(&FlashAddr,(U32*)ulTraceMemAddr, (U32*)&Spare, (U8*)&ucStatus, TRUE, TRUE);

        /* wait write command done */
        while(SUBSYSTEM_STATUS_PENDING == ucStatus)
        {
            //L3_Scheduler();

            #ifdef SIM
            //NFC_ModelSchedule();
            #endif
        }
    }

    g_TraceManagement.m_TracePhyPageRemain--;
    g_TraceManagement.m_TracePhyPos++;

    if(g_TraceManagement.m_TracePhyPos >= TRACE_BLOCK_COUNT * SUBSYSTEM_LUN_NUM * (PG_PER_BLK/2))
    {
        g_TraceManagement.m_TracePhyPos = 0;
    }

    if(MCU0_ID != ulMcuId)
    {
        /* Invalid has flush memory for MCU1/2 */
        TL_InvalidateFlushMemory(ulCurFlushSecCnt);
    }

    DBG_Printf("MCU%d FW_TraceBlockSave CurFlushSecCnt 0x%x(Pu %d Blk 0x%x Pg 0x%x) Done!\n",
        ulMcuId-1 ,ulCurFlushSecCnt,FlashAddr.m_PUSer,FlashAddr.m_BlockInPU,FlashAddr.m_PageInBlock);
#endif
}

 void MCU12_DRAM_TEXT FW_FlushMcu0Trace(SCMD* pSCMD)
 {
     U32 CriticalFlag = pSCMD->tViaDevCtrl.ucCriticalFlag;
     static MCU12_VAR_ATTR U32 ulSaveSize;
     static MCU12_VAR_ATTR U32 ulMcu0TotalTraceSize;
     TL_INFO* pTlInfo;

     g_TraceManagement.m_TraceStage = TRACE_INIT;

     do
     {
         switch (g_TraceManagement.m_TraceStage)
         {
         case TRACE_INIT:
             ulSaveSize = 0;
             pTlInfo = TL_GetTLInfo(MCU0_ID);
             ulMcu0TotalTraceSize = pTlInfo->ulTlMemSize;
             g_TraceManagement.m_TraceStage = TRACE_PREPARE;
             break;

         case TRACE_PREPARE:
             if (TRUE == g_TraceManagement.m_NeedRebuild)
             {
                 g_TraceManagement.m_TraceStage = TRACE_REBUILD;
             }
             else if (ulSaveSize >= ulMcu0TotalTraceSize)
             {
                 g_TraceManagement.m_TraceStage = TRACE_BLOCK_SAVE_DONE;
                 ulSaveSize = 0;
             }
             else
             {
                 if(0 == g_TraceManagement.m_TracePhyPageRemain)
                 {
                     g_TraceManagement.m_TraceStage = TRACE_ERASE;
                 }
                 else
                 {
                     g_TraceManagement.m_TraceStage = TRACE_SAVE;
                 }
             }
             break;

         case TRACE_REBUILD:
             FW_TraceBlockRebuild(CriticalFlag);
             g_TraceManagement.m_TraceStage = TRACE_PREPARE;
             break;

         case TRACE_ERASE:
             FW_TraceBlockErase(CriticalFlag);
             g_TraceManagement.m_TraceStage = TRACE_PREPARE;
             break;

         case TRACE_SAVE:
             FW_TraceBlockSave(MCU0_ID,CriticalFlag,ulSaveSize);
             ulSaveSize += BUF_SIZE;
             if (ulSaveSize >= ulMcu0TotalTraceSize)
             {
                 ulSaveSize = 0;
                 g_TraceManagement.m_TimeStamp++;
                 g_TraceManagement.m_TraceStage = TRACE_BLOCK_SAVE_DONE;
             }
             else
             {
                 g_TraceManagement.m_TraceStage = TRACE_PREPARE;
             }

             break;

         case TRACE_BLOCK_SAVE_DONE:
             break;

         default :
             break;
         }
     }while (g_TraceManagement.m_TraceStage != TRACE_BLOCK_SAVE_DONE);

 }

/*****************************************************************************
 Prototype      : FW_FlushSubsystemTrace
 Description    : save all trace buffer into flash
 Input          : void
 Output         : None
 Return Value   : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2013/9/13
   Author       : henryluo
   Modification : Created function

*****************************************************************************/
 void MCU12_DRAM_TEXT FW_FlushSubsystemTrace(SCMD* pSCMD)
 {
 #if 0
     U32 CriticalFlag;
     U32 ulMcuId = HAL_GetMcuId();

     if(NULL == pSCMD)
     {
         CriticalFlag = FALSE;
     }
     else
     {
         CriticalFlag = pSCMD->tViaDevCtrl.ucCriticalFlag;
     }

     FW_TraceManagementInit();

     do
     {
         switch (g_TraceManagement.m_TraceStage)
         {
         case TRACE_PREPARE:
             if (TRUE == g_TraceManagement.m_NeedRebuild)
             {
                 g_TraceManagement.m_TraceStage = TRACE_REBUILD;
             }
             else if (TRUE == TL_IsFlushMemoryEmpty())
             {
                 g_TraceManagement.m_TraceStage = TRACE_BLOCK_SAVE_DONE;
             }
             else
             {
                 if(0 == g_TraceManagement.m_TracePhyPageRemain)
                 {
                     g_TraceManagement.m_TraceStage = TRACE_ERASE;
                 }
                 else
                 {
                     g_TraceManagement.m_TraceStage = TRACE_SAVE;
                 }
             }
             break;

         case TRACE_REBUILD:
             FW_TraceBlockRebuild(CriticalFlag);
             g_TraceManagement.m_TraceStage = TRACE_PREPARE;
             break;

         case TRACE_ERASE:
             FW_TraceBlockErase(CriticalFlag);
             g_TraceManagement.m_TraceStage = TRACE_PREPARE;
             break;

         case TRACE_SAVE:
             FW_TraceBlockSave(ulMcuId,CriticalFlag,0);
             if (TRUE == TL_IsFlushMemoryEmpty())
             {
                 g_TraceManagement.m_TimeStamp++;
                 g_TraceManagement.m_TraceStage = TRACE_BLOCK_SAVE_DONE;
             }
             else
             {
                 g_TraceManagement.m_TraceStage = TRACE_PREPARE;
             }

             break;

         case TRACE_BLOCK_SAVE_DONE:
             break;

         default :
             break;
         }
     }while (g_TraceManagement.m_TraceStage != TRACE_BLOCK_SAVE_DONE);
 #endif
 }
#endif

/********************** FILE END ***************/

