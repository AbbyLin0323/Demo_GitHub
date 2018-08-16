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
Filename    :L0_Init.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_Inc.h"
#include "HAL_Interrupt.h"
#include "Disk_Config.h"
#include "L0_Interface.h"
#include "L0_ATALibAdapter.h"
#include "L0_ATAGenericCmdLib.h"
#include "L0_HcmdChain.h"
#include "L0_Init.h"
#include "L0_Schedule.h"
#include "L0_Pio.h"
#include "L0_SataErrorHandling.h"

extern U32 g_ulATAGPLBuffStart;
extern U32 g_ulATAInfoIdfyPage;
extern U32 g_ulSmartDataBaseAddr;
extern BOOL g_bMultipleDataOpen;
extern U32 L0_RAMAlloc(U32 ulRAMBaseAddr, U32 ulAllocLen, U32 *pStartPtr, U32 ulAlignToBits);
extern MCU0_DRAM_TEXT void L0_ATAInitIdentifyData(void);
void L0_SataInit(void)
{
    // initialize the SDC
    HAL_SataInitialize(BUF_SIZE_BITS); 

    // initialize L0 host command chain
    L0_HostCommandInit();

    // initialize L0 PIO related structures
    L0_PioInit();

    L0_SataEhInit();

    return;
}

BOOL L0_SataWaitBootInit(void *pParam)
{

    // initialize the identify device data, we must initialize
    // the identify device data after L0 issues boot scmd to L1,
    // since we have to use data in host info page
    L0_ATAInitIdentifyData();

    L0_ATAInitGPLog();
    L0_ATAInitSecurity();
    L0_ATAInitHPA();
    // please note that all init functions for these special
    // commands(smart, GPL, HPA, etc) should be placed
    // after the identify device init function, since they
    // will change the content of the identify device data
    L0_ATAInitSmartData();

    // init power status and timer
    L0_ATAInitPowerStatus();

    g_bMultipleDataOpen = FALSE;//TODO: load default value of this flag from NV media ?

    // Requests subsystems to initialize vendor defined command processing
    L0_ViaCmdInit();
    L0_FwUpdateInit();

    //enable SDC interrupt source after sub-system boot up
    HAL_InitInterrupt(TOP_INTSRC_SDC, BIT_ORINT_SDC);

    return TRUE;
}

BOOL L0_SataDebugModeInit(void *pParam)
{
    // initialize L0 host command chain
    L0_HostCommandInit();

    // initialize L0 PIO related structures
    L0_PioInit();

    // initialize error handling
    L0_SataEhInit();

    //re-initialize ATA param, including disk size reported in identify device data
    L0_SataWaitBootInit(NULL);

    return TRUE;
}

U32 L0_SataMemAlloc(U32 ulStartAddr)
{
    U32 ulCurrAllocPos = ulStartAddr;

    /* 1. The buffer space allocated for General Purpose Log pages. One whole buffer block in DDR SDRAM. */
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos, 
        ATA_GPL_BUFFSIZE,
        &g_ulATAGPLBuffStart,
        BUF_SIZE_BITS);

    /* 2. The buffer for SMART logs. */
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        BUF_SIZE,
        &g_ulSmartDataBaseAddr,
        BUF_SIZE_BITS);

    /* 3. Calculating the starting address of IDENTIFY DEVICE data, which belongs to General Purpose Log. */
    g_ulATAInfoIdfyPage = L0_GPLGetLogPageAddr(g_ulATAGPLBuffStart, GPL_LOGADDR_IDFYDATA, GPL_IDFYDATA_PAGE_IDCOPY);

    return ulCurrAllocPos;
}
