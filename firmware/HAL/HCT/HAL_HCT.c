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
Filename    : HAL_HCT.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.07.14
Description : this file encapsulate HCT driver interface
Others      : 
Modify      :
20140714    Gavin     Create file
20140912    Kristin   Coding style uniform
*******************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_HCT.h"
#include "HAL_SGE.h"
#include "HAL_MultiCore.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#ifdef SIM
#include "windows.h"
#include "Sim_HCT.h"
#endif

LOCAL volatile HCT_FCQ_REG *l_pHwFCQReg;
LOCAL volatile HCT_WBQ_REG *l_pHwWBQReg;
LOCAL volatile HCT_BAINC_REG *l_pHwBAIncReg;
LOCAL volatile HCT_CSCS_REG *l_pHwCSTSrchReg;

LOCAL volatile HCT_FCQ_WBQ *l_pFCQEntry;
LOCAL PWBQTABLE l_pWBQEntry;

/*------------------------------------------------------------------------------
Name: HAL_HCTInit
Description: 
    Initialize HCT related registers
Input Param:
    HCT_INIT_PARAM *pInitParam
Output Param:
    none
Return Value:
    void
Usage:
    L0 call this function when initialize
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_HCTInit(const HCT_INIT_PARAM *pInitParam)
{
    U32 ulLoop;

    /* Temporary: Shuts down clock gating for SGE status inquery. */
    rModeClkEn = 0x7F00;

    /* 1. Assigns hardware register addresses to pointers for accessing them. */
    l_pHwFCQReg = &rHCT_FCQ_REG;
    l_pHwWBQReg = &rHCT_WBQ_REG;
    l_pHwBAIncReg = (volatile HCT_BAINC_REG *)HCT_DSBA_REG_BASE;
    l_pHwCSTSrchReg = &rHCT_CSCS_REG;

    /* 2. Assigns FCQ/WBQ base address to pointers for accessing them. */
    l_pFCQEntry = (volatile HCT_FCQ_WBQ *)pInitParam->tFCQParam.ulBaseAddr;
    l_pWBQEntry = (PWBQTABLE)pInitParam->tWBQParam.ulBaseAddr;

    /* 3. Programs hardware register block for SRAM base addresses and parameters. */
    for (ulLoop = 0; ulLoop <= 4; ulLoop++)
    {
        l_pHwBAIncReg[ulLoop].ulAsU32 = 0;
        l_pHwBAIncReg[ulLoop].usBA = (U16)(pInitParam->aSRAMTableParam[ulLoop].ulBaseAddr - HCT_SRAM_BASE);
        l_pHwBAIncReg[ulLoop].bsINC = (U16)(pInitParam->aSRAMTableParam[ulLoop].ulIncrement);
    }

    /* 4. Programs hardware register block for FCQ/WBQ base addresses and parameters. */
    l_pHwFCQReg->usFCQBA = (U16)(pInitParam->tFCQParam.ulBaseAddr - HCT_SRAM_BASE);

    l_pHwWBQReg->usWBQBA = (U16)(pInitParam->tWBQParam.ulBaseAddr - HCT_SRAM_BASE);
    l_pHwWBQReg->bsWBQTRI = (U8)pInitParam->tWBQParam.usTriggerState;
    l_pHwWBQReg->ucWBQINC = (U8)pInitParam->tWBQParam.usIncrement;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTReset
Description: 
    process HCT reset bit(that is, bit 23 of register GLB18) to reset HCT HW
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    error handling
History:
20141104    Kristin    create
------------------------------------------------------------------------------*/
void HAL_HCTReset(void)
{
    HAL_HCTAssertReset();
    HAL_DelayCycle(20);  //wait 20 clock
    HAL_HCTReleaseReset();

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTAssertReset
Description: 
    Asserts HCT reset bit(that is, bit 23 of register GLB18) to reset HCT HW
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    error handling
History:
20151214 Yao Chen added
------------------------------------------------------------------------------*/
void HAL_HCTAssertReset(void)
{
    rGlbSoftRst |= (R_RST_HCT | R_RST_HOSTC); //set HCT reset bit to 1
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTReleaseReset
Description: 
    Releases HCT reset bit(that is, bit 23 of register GLB18) to reset HCT HW
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    error handling
History:
20151214 Yao Chen added
------------------------------------------------------------------------------*/
void HAL_HCTReleaseReset(void)
{
    rGlbSoftRst &= ~(R_RST_HCT | R_RST_HOSTC); //clear HCT reset bit to 0
    return;    
}

/*------------------------------------------------------------------------------
Name: HAL_HCTClearAutoReset
Description: 
    Releases HCT reset when HCT is reset by hardware in a PCIE reset.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    error handling
History:
20151214 Yao Chen added
------------------------------------------------------------------------------*/
void HAL_HCTClearAutoReset(void)
{
    rPMU_HCT_CLRRST &= ~(HOSTC_RESET_PENDING | HCT_RESET_PENDING);

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTSetCST
Description: 
    set current status of a command slot into the CST register
Input Param:
    U32 ulCmdId: command slot id 
    U32 ulCurrState: current status to set
Output Param:
    none
Return Value:
    U32: last status of this command slot
Usage:
    1. Trigger WBQ
    2. FW updates command status
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
U32 HAL_HCTSetCST(U32 ulCmdId, U32 ulCurrState)
{
    U32 ulPrevState;

    ulPrevState = rHCT_CS_REG[ulCmdId];
#ifdef SIM
    HCT_SetCSTByFw(ulCmdId, (U8)ulPrevState, (U8)ulCurrState);
#else
    rHCT_CS_REG[ulCmdId] = (U8)ulCurrState;
#endif

    return ulPrevState;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTGetCST
Description: 
    get current status of a command slot from the CST register
Input Param:
    U32 ulCmdId: command slot id 
Output Param:
    none
Return Value:
    U32: the value in CST register of the specified command slot
Usage:
    check status of a command slot
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
U32 HAL_HCTGetCST(U32 ulCmdId)
{
    return rHCT_CS_REG[ulCmdId];
}

/*------------------------------------------------------------------------------
Name: HAL_HCTSearchCST
Description: 
    make HW to search for a command slot in the specified status by using
    Command State Control and Status Registers(CSCS)
Input Param:
    U32 ulState: the specified status
Output Param:
    none
Return Value:
    U32: the eligible slot id
Usage:
    search for a command slot in the specified status
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
U32 HAL_HCTSearchCST(U32 ulState)
{
    U32 ulCmdId;

    l_pHwCSTSrchReg->bsCST = ulState;
    l_pHwCSTSrchReg->bsCSTTRI = TRUE;

#ifdef SIM
    HCT_SearchCST();
#endif

    while (0 == l_pHwCSTSrchReg->bsCSTRDY)
    {
        /* Waiting for HW search done,
                  while monitoring whether a PCIe reset has occured in C0 hardware. */
        if ((0 != (rPMU_HCT_CLRRST & (HOSTC_RESET_PENDING | HCT_RESET_PENDING))) ||
            (0 != (rGlbSoftRst & (R_RST_HCT | R_RST_HOSTC))))
        {
            /* A PCIe reset would cause HCT hardware to be stuck in reset state and not
                         able to complete the search operation. */
            return INVALID_CMD_ID;
        }
    }

    if (TRUE == l_pHwCSTSrchReg->bsCSTNOID)
    {
        /* find no eligible slot */
        ulCmdId = INVALID_CMD_ID;
    }
    else
    {
        ulCmdId = l_pHwCSTSrchReg->bsCSTCID;
    }

    return ulCmdId;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTGetFCQEntry
Description: 
    Get the pointer of an available FCQ entry.
Input Param:
    none
Output Param:
    none
Return Value:
    HCT_FCQ_WBQ *: the pointor of a available FCQ entry,
                   NULL indicates FCQ is full.
Usage:
    Anywhere need to fill a new FCQ
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
volatile HCT_FCQ_WBQ *HAL_HCTGetFCQEntry(void)
{
    volatile HCT_FCQ_WBQ *pCurrEntry;

    /* Yao [2015/3/2]: HCT would get unstable in B0 ASIC when running at PCIe Gen 3 speed.
            This patch can improve the stablity of HCT under the circumstance above. */
    if (TRUE == l_pHwFCQReg->bsFCQFULL)
    /* Patch end [2015/3/2]. */
    {
        /* Assigns a NULL pointer to the return value to indicate FCQ is full currently. */
        pCurrEntry = NULL;
    }
    else
    {
        /* Gets current FCQ position from hardware. */
        pCurrEntry = &l_pFCQEntry[l_pHwFCQReg->bsFCQWP];
    }

    return pCurrEntry;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTGetWBQEntry
Description: 
    get the pointer of a WBQ entry.
Input Param:
    U32 ulCmdId: command slot id
    U32 ulWBQLevel: WBQ index of this slot, 0 ~ (WBQ_N - 1)
Output Param:
    none
Return Value:
    HCT_FCQ_WBQ *: the pointor of the WBQ entry
Usage:
    Anywhere need to fill a new WBQ
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
volatile HCT_FCQ_WBQ *HAL_HCTGetWBQEntry(U32 ulCmdId, U32 ulWBQLevel)
{
    return &(l_pWBQEntry[ulCmdId][ulWBQLevel]);
}

/*------------------------------------------------------------------------------
Name: HAL_HCTPushFCQEntry
Description: 
    Trigger HW to process a FCQ.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    Anywhere after filling a new FCQ
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_HCTPushFCQEntry(void)
{
    /* Triggering hardware for FCQ entry fetching. */
#ifdef SIM
    SIM_HostCFCQWriteTrigger();
#else
    l_pHwFCQReg->bsFCQPUSH = TRUE;
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCTTriggerWBQ
Description: 
    trigger HW to process all WBQs of the specified slot 
    (from index 0, bLast in WBQ indicates the last WBQ of the slot)
Input Param:
    U32 ulCmdId: command slot id
Output Param:
    none
Return Value:
    void
Usage:
    Anywhere after filling the last WBQ of a slot
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_HCTTriggerWBQ(U32 ulCmdId)
{
    U32 ulTriggerState = l_pHwWBQReg->bsWBQTRI;

    HAL_HCTSetCST(ulCmdId, ulTriggerState);

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_TrigPCIeVDMsg
Description: 
    trigger HW to send PCIe Vendor-Define Message type 1 to Host
Input Param:
    U32 ulMsgData: Message Data Vendor_Defined
    see PCIe spec 2.2.8.6: Vendor_Defined Messages
Output Param:
    none
Return Value:
    void
Usage:
    The vendor-define message can be captured by PCIe CATC. We use it for debug.
History:
20150105    Gavin    create function
------------------------------------------------------------------------------*/
void HAL_TrigPCIeVDMsg(U32 ulMsgData)
{
    // prepare message data. HOSTC_REG_0x58
    *(volatile U32 *)(REG_BASE_HOSTC + 0x58) = ulMsgData;

    // prepare message code. HOSTC_REG_0x50 bit[23:16]
    *(volatile U32 *)(REG_BASE_HOSTC + 0x50) &= ~(0xFF << 16);
    *(volatile U32 *)(REG_BASE_HOSTC + 0x50) |= (0x7F << 16);//0x7F: Vendor Define Message Type1 

    // trigger HW. the trigger bit(HOSTC_REG_0x50 bit24) is cleared to zero by HW automatically
    *(volatile U32 *)(REG_BASE_HOSTC + 0x50) |= (1 << 24);

    return;
}
/********************** FILE END ***************/
