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
Filename    : HAL_Trax.c
Version     : Ver 1.0
Author      : tobey
Date        : 2014.11.20
Description : this file encapsulate Trax driver interface. 
Others      : 
Modify      :
20141120    Tobey     Create file
*******************************************************************************/

#include "BaseDef.h"
#include "Proj_Config.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_Trax.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"

#if defined(FPGA)  || defined(ASIC) || defined(COSIM)
#include <xtensa/tie/xt_externalregisters.h>

U32 aReadableReg[TRAX_READABLE_REG_NUM] = 
{
    TRAX_ID, 
    TRAX_CTRL, 
    TRAX_STAT, 
    TRAX_ADDR,
    TRAX_TRIGGERPC, 
    TRAX_PCMATCHCTRL, 
    TRAX_DELAYCOUNT, 
    TRAX_MEMSTARTADDR,
    TRAX_MEMENDADDR,
    
    TRAX_P4CHANGE,
    TRAX_P4REV,
    TRAX_P4DATE,
    TRAX_P4TIME,
    TRAX_PDSTATUS,
    TRAX_PDDATA,
    TRAX_MSG_STATUS,
    TRAX_FSM_STATUS,
    TRAX_IB_STATUS
};

U32 HAL_GetTraxReg (const U32 ulRegAddr)
{
    U32 ulReadData;
  /* The register number passed is the general NAR register number
   * Hence, convert it to appropriate ERI register number
   */
    ulReadData = XT_RER(ulRegAddr);

  return ulReadData;
}

/*----------------------------------------------------------------------------
Name: HAL_GetTRAXID
Description: 
    Get TRASXID value
Input Param:
    none
Output Param:
    TRAXIDREG * pTRAXIDVal: pointer of read value
Return Value:
    void
Usage:
    mcu invoke it to get TRAXID val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
TRAXIDREG HAL_GetTRAXID(void)
{
    TRAXIDREG tTraxId;
    *((U32 *)(&tTraxId)) = XT_RER(TRAX_ID);
    
    return tTraxId;
}

/*----------------------------------------------------------------------------
Name: HAL_GetTRAXCTRL
Description:
    Get TRAXCTRL value
Input Param:
    none
Output Param:
    TRAXCTRLREG * pTRAXCTRLVal: pointer of read value.
Return Value:
    void
Usage:
    mcu invoke it to get TRAXCTR val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
TRAXCTRLREG HAL_GetTRAXCTRL(void)
{
    TRAXCTRLREG tTraxCtrl;
    
    *((U32 *)(&tTraxCtrl)) = XT_RER(TRAX_CTRL);
    
    return tTraxCtrl;
}

/*----------------------------------------------------------------------------
Name: HAL_SetTRAXCTRL
Description:
    set TRAXCTRL with value tTRAXCTRLVal.
Input Param:
    TRAXCTRLREG tTRAXCTRLVal: write value
Output Param:
    none
Return Value:
    void
Usage:
    mcu invoke it to set TRAXCTRL  with value tTRAXCTRLVal via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_SetTRAXCTRL(TRAXCTRLREG * pTRAXCTRLVal)
{
    XT_WER(*((U32 *)pTRAXCTRLVal), TRAX_CTRL);
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GetTRAXSTAT
Description:
    Get TRAXSTAT value
Input Param:
    none
Output Param:
    TRAXSTATREG * pTRAXSTATVal: pointer of read value.
Return Value:
    void
Usage:
    mcu invoke it to get TRAXSTAT val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
TRAXSTATREG HAL_GetTRAXSTAT(void)
{
    TRAXSTATREG tTraxStat;
    *((U32 *)(&tTraxStat)) = XT_RER(TRAX_STAT);

    return tTraxStat;
}

/*----------------------------------------------------------------------------
Name: HAL_GetTRIGGERPC
Description:
    Get TRIGGERPC(trace stop pc) value, can read at anytime.
Input Param:
    none
Output Param:
    U32 * pTRIGGERPCVal: pointer of read value.
Return Value:
    void
Usage:
    mcu invoke it to get TRIGGERPC val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U32 HAL_GetTRIGGERPC()
{
    U32 pTrigPCVal;
    
    pTrigPCVal = XT_RER(TRAX_TRIGGERPC);
    return pTrigPCVal;
}

/*----------------------------------------------------------------------------
Name: HAL_SetTRIGGERPC
Description:
    Set TRIGGERPC(trace stop pc) value, can writ at anytime. the timing of writes
    made while tracing is axtive is not defined.
Input Param:
    none
Output Param:
    U32 ulTRIGGERPCVal: set value.
Return Value:
    void
Usage:
    mcu invoke it to set TRIGGERPC val with ulTRIGGERPCVal via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_SetTRIGGERPC(U32 ulTRIGGERPCVal)
{
    XT_WER(ulTRIGGERPCVal, TRAX_TRIGGERPC);
    return;
}

U32 HAL_GetTRAXADDR(void)
{
    U32 ulTraxAddrVal;
    
    ulTraxAddrVal = XT_RER(TRAX_ADDR);
    return ulTraxAddrVal;
}


void HAL_SetTRAXADDR(U32 ulTRAXADDRVal)
{
    XT_WER(ulTRAXADDRVal, TRAX_ADDR);
    return;
}

U32 HAL_GetTRAXDATA(void)
{
    U32 ulTraxData;
    
    ulTraxData = XT_RER(TRAX_DATA);
    return ulTraxData;
}

/*----------------------------------------------------------------------------
Name: HAL_GetPCMATCHCTRL
Description:
    Get PCMATCHCTRL value, can read at anytime.
Input Param:
    none
Output Param:
    PCMATCHCTRLREG * pPCMATCHCTRLVal: pointer of read value.
Return Value:
    void
Usage:
    mcu invoke it to get PCMATCHCTRL val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
PCMATCHCTRLREG HAL_GetPCMATCHCTRL(void)
{
    PCMATCHCTRLREG tPCMatch;
    
    *((U32 *)(&tPCMatch)) = XT_RER(TRAX_PCMATCHCTRL);   
    return tPCMatch;
}

/*----------------------------------------------------------------------------
Name: HAL_SetPCMATCHCTRL
Description:
    Set PCMATCHCTRL value, can writ at anytime. the timing of writes
    made while tracing is axtive is not defined.
Input Param:
    none
Output Param:
    PCMATCHCTRLREG ulPCMATCHCTRLVal: set value.
Return Value:
    void
Usage:
    mcu invoke it to set PCMATCHCTRL val with ulPCMATCHCTRLVal via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_SetPCMATCHCTRL(PCMATCHCTRLREG * pPCMATCHCTRLVal)
{
    XT_WER(*((U32 *)pPCMATCHCTRLVal), TRAX_PCMATCHCTRL);
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GetDELAYCOUNT
Description:
    Get DELAYCOUNT(post stop trigger capture size) value.
    only the lower 24 bits are significant, upper 8 bits are reserved.
Input Param:
    none
Output Param:
    U32 * pDELAYCOUNTVal: pointer of read value.
Return Value:
    void
Usage:
    mcu invoke it to get DELAYCOUNT val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U32 HAL_GetDELAYCOUNT(void)
{
    U32 ulDelayCnt;
    
    ulDelayCnt = XT_RER(TRAX_DELAYCOUNT);
    return ulDelayCnt;
}

/*----------------------------------------------------------------------------
Name: HAL_SetDELAYCOUNT
Description:
    Set DELAYCOUNT value,
    only the lower 24 bits are significant, upper 8 bits are reserved.
Input Param:
    none
Output Param:
    U32 ulDELAYCOUNTVal: set value.
Return Value:
    void
Usage:
    mcu invoke it to set DELAYCOUNT val with ulDELAYCOUNTVal via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_SetDELAYCOUNT(U32 ulDELAYCOUNTVal)
{
    //only lower 24-bits of the register are significant
    if(ulDELAYCOUNTVal >= (1<<24))
    {
        DBG_Getch("DELAYCOUNT setting error!\n");
    }
    XT_WER(ulDELAYCOUNTVal, TRAX_DELAYCOUNT);
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GetMEMSTARTADDR
Description:
    Get MEMSTARTADDR of TraceRAM value.
    only the lower m bits are significant, same with TRAXADDR.TADDR
Input Param:
    none
Output Param:
    U32 * pMEMSTARTADDRVal: pointer of read value.
Return Value:
    void
Usage:
    mcu invoke it to get MEMSTARTADDR val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U16 HAL_GetMEMSTARTADDR(void)
{
    U16 usMemStartAddr;
    U32 ulMemAddrTemp;
    
    ulMemAddrTemp = XT_RER(TRAX_MEMSTARTADDR);
    usMemStartAddr = (~(INVALID_8F<<m)) & ulMemAddrTemp;
    
    return usMemStartAddr;
}

/*----------------------------------------------------------------------------
Name: HAL_SetMEMSTARTADDR
Description:
    Set MEMSTARTADDR value,
    only the lower m bits are significant, same with TRAXADDR.TADDR
Input Param:
    none
Output Param:
    U32 ulMEMSTARTADDRVal: set value.
Return Value:
    void
Usage:
    mcu invoke it to set MEMSTARTADDR val with ulMEMSTARTADDRVal via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_SetMEMSTARTADDR(U16 usMEMSTARTADDRVal)
{
    U32 ulMemAddrTemp;
    
    if(usMEMSTARTADDRVal > (1<<m))
    {
        DBG_Getch();
    }
    
    ulMemAddrTemp = XT_RER(TRAX_MEMSTARTADDR);
    ulMemAddrTemp = (ulMemAddrTemp & (INVALID_8F<<m)) | usMEMSTARTADDRVal;
    XT_WER(ulMemAddrTemp, TRAX_MEMSTARTADDR);
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GetMEMENDADDR
Description:
    Get MEMENDADDR of TraceRAM value.
    only the lower m bits are significant, same with TRAXADDR.TADDR
Input Param:
    none
Output Param:
    U32 * pMEMENDADDRVal: pointer of read value.
Return Value:
    void
Usage:
    mcu invoke it to get MEMENDADDR val via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U16 HAL_GetMEMENDADDR(void)
{
    U16 usMemEndAddr;
    U32 ulMemAddrTemp;
    
    ulMemAddrTemp = XT_RER(TRAX_MEMENDADDR);
    usMemEndAddr = (~(INVALID_8F<<m)) & ulMemAddrTemp;
    
    return usMemEndAddr;
}

/*----------------------------------------------------------------------------
Name: HAL_SetMEMENDADDR
Description:
    Set MEMENDADDR value,
    only the lower m bits are significant, same with TRAXADDR.TADDR
Input Param:
    none
Output Param:
    U32 ulMEMENDADDRVal: set value.
Return Value:
    void
Usage:
    mcu invoke it to set MEMENDADDR val with ulMEMENDADDRVal via ERI bus.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_SetMEMENDADDR(U16 ulMEMENDADDRVal)
{
    U32 ulMemAddrTemp;
    
    if(ulMEMENDADDRVal > (1<<m))
    {
        DBG_Getch();
    }

    ulMemAddrTemp = XT_RER(TRAX_MEMENDADDR);
    ulMemAddrTemp = (ulMemAddrTemp & (INVALID_8F<<m)) | ulMEMENDADDRVal;
    XT_WER(ulMemAddrTemp, TRAX_MEMENDADDR);
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxIsTraceActive
Description:
    check trax whether active or not.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: 1:active 0:not active
Usage:
    mcu invoke it check trax whether active or not.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxIsTraceActive(void)
{
    TRAXSTATREG tTraxStat;
    
    tTraxStat = HAL_GetTRAXSTAT();
    return tTraxStat.bsTRACT;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxIsTraceStoped
Description:
    check trax whether trax stop or not.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:trace stoped FALSE:trace not stoped
Usage:
    mcu invoke it check trax whether trax stop or not.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxIsTraceStoped(void)
{
    TRAXSTATREG tTraxStat;
    
    tTraxStat = HAL_GetTRAXSTAT();
    return tTraxStat.bsTRIG;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxIsPCMatchStopTrig
Description:
    check current stop trigger is PCMATCHStopTrig
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:PCMATCHStopTrig FALSE:not PCMATCHStopTrig
Usage:
    mcu invoke it check current stop trigger is PCMATCHStopTrig
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxIsPCMatchStopTrig(void)
{
    TRAXSTATREG tTraxStat;
    
    tTraxStat = HAL_GetTRAXSTAT();
    return tTraxStat.bsPCMTG;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxIsPTIStopTrig
Description:
    check current stop trigger is PTIStopTrig
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:PTIStopTrig FALSE:not PTIStopTrig
Usage:
    mcu invoke it check current stop trigger is PTIStopTrig
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxIsPTIStopTrig(void)
{
    TRAXSTATREG tTraxStat;
    
    tTraxStat = HAL_GetTRAXSTAT();
    return tTraxStat.bsPTITG;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxIsCTIStopTrig
Description:
    check current stop trigger is CTIStopTrig
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:CTIStopTrig FALSE:not CTIStopTrig
Usage:
    mcu invoke it check current stop trigger is CTIStopTrig
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxIsCTIStopTrig(void)
{
    TRAXSTATREG tTraxStat;
    
    tTraxStat = HAL_GetTRAXSTAT();
    return tTraxStat.bsCTITG;
}

LOCAL U32 TraxPowerOf2(U8 ucPowerNum)
{
    U32 ulRamSize;
    
    ulRamSize = 1<<ucPowerNum;
    return ulRamSize;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetTraceRamSize
Description:
    get TraceRAM size.
Input Param:
    none
Output Param:
    none
Return Value:
    U32:TraceRAM size.
Usage:
    mcu invoke it get TraceRAM size.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U32 HAL_TraxGetTraceRamSize(void)
{
    U8 ucPower;
    U32 ulRamSize = 1;
    TRAXSTATREG tTraxStat;
    
    tTraxStat = HAL_GetTRAXSTAT();
    ucPower = tTraxStat.bsMEMSZ;
    ulRamSize = TraxPowerOf2(ucPower);

    return ulRamSize;
}


/*----------------------------------------------------------------------------
Name: HAL_TraxGetITCTOA
Description:
    get current CrossTriggerOutAck input observation value used in integration mode
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:CrossTriggerOutAck happened FALSE: NO CrossTriggerOutAck happen
Usage:
    invoke it to get current CrossTriggerOutAck 
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetITCTOA(void)
{
    TRAXSTATREG tTraxStat;
    tTraxStat = HAL_GetTRAXSTAT();

    return tTraxStat.bsITCTOA;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetITCTI
Description:
    get current CTI input observation value used in integration mode.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:CTI occur; FALSE:CTI not happen
Usage:
    invoke it CTI
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetITCTI(void)
{
    TRAXSTATREG tTraxStat;
    tTraxStat = HAL_GetTRAXSTAT();

    return tTraxStat.bsITCTI;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetITCTI
Description:
    get current CTI input observation value used in integration mode.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:CTI occur; FALSE:CTI not happen
Usage:
    invoke it get current CTI.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetITATR(void)
{
    TRAXSTATREG tTraxStat;
    tTraxStat = HAL_GetTRAXSTAT();

    return tTraxStat.bsITATR;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetCTO
Description:
    get current trax CTO sts.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: This bit reflects the current value of CTO. It is cleared when TRAXCTRL.TREN transitions
    from 1 to 0, or when CTOWT or CTOWS bits that make it set are cleared. It is set while
    TRAXCTRL.TREN is set when either: TRAXCTRL.CTOWT is set and the stop trigger
    fires, or TRAXCTRL.CTOWS is set and trace output stops. CTO is then also cleared when
    the CrossTriggerOutAck signal is received.
Usage:
    get current trax CTO sts.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetCTO(void)
{
    TRAXSTATREG tTraxStat;
    tTraxStat = HAL_GetTRAXSTAT();

    return tTraxStat.bsCTO;
}


/*----------------------------------------------------------------------------
Name: HAL_TraxGetPTO
Description:
    get current trax PTO sts.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL:This bit reflects the current value of PTO. It is cleared when TRAXCTRL.TREN
    transitions from 1 to 0, or when PTOWT or PTOWS bits that make it set are cleared. It is set
    while TRAXCTRL.TREN is set when either: TRAXCTRL.PTOWT is set and the stop
    trigger fires, or TRAXCTRL.PTOWS is set and trace output stops.
    PTO is an internal signal that is latched into OCD register bit DSR.DebugPendTrax
    when a TRAX trigger causes a debug interrupt.
Usage:
    mcu invoke it get trax PTO sts.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetPTO(void)
{
    TRAXSTATREG tTraxStat;
    tTraxStat = HAL_GetTRAXSTAT();

    return tTraxStat.bsPTO;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetCTIStop
Description:
    set trax CTIStop.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to set trax CTIStop.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetCTIStop(void)
{
    TRAXCTRLREG tTraxCtrl;
    
    tTraxCtrl = HAL_GetTRAXCTRL();
    tTraxCtrl.bsCTIEN = TRUE;
    HAL_SetTRAXCTRL(&tTraxCtrl);
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetCTIStop
Description:
    get trax CTIStop.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to get trax CTIStop.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetCTIStop(void)
{
    TRAXCTRLREG tTraxCtrl;
    
    tTraxCtrl = HAL_GetTRAXCTRL();
    return tTraxCtrl.bsCTIEN;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetCTOWhen
Description:
    set CTO condition.
Input Param:
    U8 ucCondition: 0:off; 1:OnTrig; 2:OnHalt;
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to set CTO condition.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetCTOWhen(U8 ucCondition)
{
    TRAXCTRLREG tTraxCtrl;
    
    if(ucCondition <= 2)
    {
        tTraxCtrl = HAL_GetTRAXCTRL();
        tTraxCtrl.bsCTOWHEN = ucCondition;
        HAL_SetTRAXCTRL(&tTraxCtrl);
    }
    else
    {
        DBG_Getch();
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetCTOWhen
Description:
    get CTO condition.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: 0:off; 1:OnTrig; 2:OnHalt;
Usage:
    invoke it to get CTO condition.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U8 HAL_TraxGetCTOWhen(void)
{
    TRAXCTRLREG tTraxCtrl;
    
    tTraxCtrl = HAL_GetTRAXCTRL();
    return (U8)tTraxCtrl.bsCTOWHEN;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetPTIStop
Description:
    set trax PTIStop.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to set trax PTIStop.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetPTIStop(void)
{
    TRAXCTRLREG tTraxCtrl;
    
    tTraxCtrl = HAL_GetTRAXCTRL();
    tTraxCtrl.bsPTIEN= TRUE;
    HAL_SetTRAXCTRL(&tTraxCtrl);
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetPTIStop
Description:
    get trax PTIStop.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to get trax PTIStop.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetPTIStop(void)
{
    TRAXCTRLREG tTraxCtrl;
    
    tTraxCtrl = HAL_GetTRAXCTRL();

    return tTraxCtrl.bsPTIEN;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetPTOWhen
Description:
    set PTO condition.
Input Param:
    U8 ucCondition: 0:off; 1:OnTrig; 2:OnHalt;
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to set PTO condition.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetPTOWhen(U8 ucCondition)
{
    TRAXCTRLREG tTraxCtrl;

    if(ucCondition <= 2)
    {
        tTraxCtrl = HAL_GetTRAXCTRL();
        tTraxCtrl.bsPTOWHEN = ucCondition;
        HAL_SetTRAXCTRL(&tTraxCtrl);
    }
    else
    {
        DBG_Getch();
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetPTOWhen
Description:
    get PTO condition.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: 0:off; 1:OnTrig; 2:OnHalt;
Usage:
    invoke it to get PTO condition.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U8 HAL_TraxGetPTOWhen(void)
{
    TRAXCTRLREG tTraxCtrl;
    
    tTraxCtrl = HAL_GetTRAXCTRL();
    return (U8)tTraxCtrl.bsPTOWHEN;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetSMPER
Description:
    This function selects the trace synchronization message period accordint to TraceRAM size.
    bsSMPER:0 = off, 1 = on, -1 = auto, 8, 16, 32, 64, 128, 256
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:CTI occur; FALSE:CTI not happen
Usage:
    invoke it selects the trace synchronization message period.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetSMPER(U32 ulTraxSize)
{
    TRAXCTRLREG tTraxCtrl;
    tTraxCtrl = HAL_GetTRAXCTRL();

    if(ulTraxSize <= 256)
    {
        tTraxCtrl.bsSMPER = 5;
    }
    else if((ulTraxSize > 256) && (ulTraxSize <= 1024))
    {
        tTraxCtrl.bsSMPER = 4;
    }
    else if((ulTraxSize > 1024) && (ulTraxSize <= 4096))
    {
        tTraxCtrl.bsSMPER = 3;
    }
    else if((ulTraxSize > 4096) && (ulTraxSize <= 16384))
    {
        tTraxCtrl.bsSMPER = 2;
    }
    else 
    {
        tTraxCtrl.bsSMPER = 1;
    }

    HAL_SetTRAXCTRL(&tTraxCtrl);
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxInitERI
Description:
    Init Trax via ERI.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    mcu invoke it to Init Trax via ERI.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxInitERI(TRAX_CONTEXT *pContext)
{
    U32 ulRamSize;
    TRAXIDREG tTraxId;

    tTraxId = HAL_GetTRAXID();

    if((0 == *(U32 *)&tTraxId) || (INVALID_8F== *(U32 *)&tTraxId))
    {
        DBG_Getch();
    }

    pContext->usTraxVersion= (tTraxId.bsMAJVER<<3) | tTraxId.bsMINVER;
    ulRamSize = HAL_TraxGetTraceRamSize();

    if ((ulRamSize < TRAXRAM_SIZE_MIN) || (ulRamSize > TRAXRAM_SIZE_MAX))
    {
        DBG_Getch();
        DBG_Printf("Trax Ram Size error\n");
    }

    pContext->ulTraxTramSize= ulRamSize;

    pContext->ulFlags= TRAX_FHEADF_OCD_ENABLED;

    pContext->usAddrReadLast= 0;

    /* Lets assume initially that the total memory traced is
    * the trace RAM size */
    pContext->ulTotalMemlen= pContext->ulTraxTramSize;  
    
    return;
}


/*----------------------------------------------------------------------------
Name: HAL_TraxStart
Description:
    stop trax.
Input Param:
    U8 ucTraxStopType: 0x01:TRAX_STOP_HALT; 0x02:TRAX_STOP_QUIET;
Output Param:
    none
Return Value:
    none
Usage:
    invoke it stop trax.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxStopHalt(void)
{ 
    TRAXCTRLREG tTraxCtrl;
  
    if (FALSE == HAL_TraxIsTraceActive())
    {
        return;
    }
    else
    {
        /* forcefully stop trace immediately while trax is active */
        tTraxCtrl = HAL_GetTRAXCTRL();
        if(FALSE == tTraxCtrl.bsTRSTP)
        {
            tTraxCtrl.bsTRSTP = TRUE;
            HAL_SetTRAXCTRL(&tTraxCtrl);
        }

        HAL_SetDELAYCOUNT(0);

        while(FALSE != HAL_TraxIsTraceActive())
        {
            ;
        }
        tTraxCtrl = HAL_GetTRAXCTRL();
        tTraxCtrl.bsTREN = FALSE;
        HAL_SetTRAXCTRL(&tTraxCtrl);
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxReset
Description:
    stop trax. and set all tarx registers to default value. 
Input Param:
    TRAX_CONTEXT * pContext:
Output Param:
    none
Return Value:
    none
Usage:
    invoke it stop trax and prepare for a new start.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxReset(TRAX_CONTEXT * pContext)
{   
    TRAXCTRLREG tTraxCtrl;
    PCMATCHCTRLREG tPCMATCHCTRLVal;

    COM_MemZero((U32 *)&tPCMATCHCTRLVal, sizeof(PCMATCHCTRLREG));
    COM_MemZero((U32 *)&tTraxCtrl, sizeof(TRAXCTRLREG));
    tTraxCtrl.bsTMEN = TRUE;
    tTraxCtrl.bsSMPER = 0x1;
    
    HAL_TraxStopHalt();
    HAL_SetTRAXCTRL(&tTraxCtrl);
    HAL_SetTRAXADDR(0);
    HAL_SetTRIGGERPC(0);
    HAL_SetPCMATCHCTRL(&tPCMATCHCTRLVal);
    HAL_SetDELAYCOUNT(0);
    HAL_SetMEMSTARTADDR(0);
    HAL_SetMEMENDADDR((pContext->ulTraxTramSize >> 2) - 1);

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxStart
Description:
    let trax start.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: TRUE:start ok; FALSE:tarx is active now
Usage:
    invoke it let trax start.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxStart(void)
{ 
    TRAXCTRLREG tTraxCtrl;
    
    if(FALSE == HAL_TraxIsTraceActive())
    {
        tTraxCtrl = HAL_GetTRAXCTRL();
        tTraxCtrl.bsTREN = FALSE;
        HAL_SetTRAXCTRL(&tTraxCtrl);

        tTraxCtrl = HAL_GetTRAXCTRL();
        tTraxCtrl.bsTREN= TRUE;
        HAL_SetTRAXCTRL(&tTraxCtrl);
    }
    else
    {
        DBG_Printf("It's tracing now\n");
        return FALSE;
    }
    
    return TRUE;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetRamBoundary
Description:
    set TraceRAM trace boundary. the effective size of the circular buffer area must
    be at minimum 64 bytes. there is no aliganment requirement.
Input Param:
    U32 ulStartAddr:TraceRamStartAddr
    U32 ulEndAddr:TraceRamEndAddr
    the TraceRAM memory size for trace is "ulEndAddr - ulStartAddr + 1"
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to set TraceRAM trace boundary
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetRamBoundary(TRAX_CONTEXT *pContext, U16 usStartAddr, U16 usEndAddr)
{
    S16 ssTraceSize;
    U16 usStartAddrRd;
    U16 usEndAddrRd;
    if (( usStartAddr > (pContext->ulTraxTramSize >> 2) - 1)
       || ( usEndAddr > (pContext->ulTraxTramSize >> 2) - 1))
    {
        DBG_Printf("trax addr is out of range\n");
        DBG_Getch();
    }
    
    ssTraceSize = (S16)usEndAddr - (S16)usStartAddr;
    if (ssTraceSize < 0)
    {
       ssTraceSize += ((pContext->ulTraxTramSize >> 2) - 1);
    }
    /* In terms of bytes */
    ssTraceSize = (ssTraceSize + 1) << 2;
    
    if (ssTraceSize < TRAX_MIN_TRACEMEM)
    {
        DBG_Printf("trax size setting error\n");
        DBG_Getch();
    }

    if(FALSE == HAL_TraxIsTraceActive())
    {
        HAL_SetMEMSTARTADDR(usStartAddr);
        HAL_SetMEMENDADDR(usEndAddr);
    }
    else
    {
        DBG_Printf("Trax is busy, can't set ram boundary!\n");
    }

    /* If memory shared option is not configured, the start and the end address
      * of the trace RAM are set to default, so there is a need to read them back
      * and check */
     usStartAddrRd = HAL_GetMEMSTARTADDR();
     usEndAddrRd = HAL_GetMEMENDADDR();
    
     if ((usStartAddrRd != usStartAddr) || (usEndAddrRd != usEndAddr))
     {
        DBG_Getch();
        DBG_Printf("trace boundary setting fail!\n");
     }

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetRamBoundary
Description:
    get TraceRAM trace boundary. the effective size of the circular buffer area must
    be at minimum 64 bytes. there is no aliganment requirement.
Input Param:
    U32 * pStartAddr:pointer of start addr
    U32 * pEndAddr:pointer of end addr
Output Param:
    none
Return Value:
    none
Usage:
    invoke it get TraceRAM trace boundary
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxGetRamBoundary(U16 * pStartAddr, U16 * pEndAddr)
{    
    *pStartAddr = HAL_GetMEMSTARTADDR();
    *pEndAddr = HAL_GetMEMENDADDR();
 
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxGetSyncper
Description:
    get current setting of trax synchronization period.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: 0 = off,1 every 8, 16, 32, 64, 128, 256
Usage:
    invoke it to get current setting of trax synchronization period.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
U8 HAL_TraxGetSyncper(void)
{
    U8 ucSMPER;
    TRAXCTRLREG tTraxCtrl;
    
    tTraxCtrl = HAL_GetTRAXCTRL();
    ucSMPER = tTraxCtrl.bsSMPER;
    
    if (ucSMPER)
    {
      return (1 << (9 - ucSMPER));
    }
    else
    {
      return 0;
    }
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetPCStop
Description:
    set Trax PC stop range.
    ulPCStartAddr, ulPCEndAddr should meet condition listed below:
    (ulPCEndAddr - ulPCStartAddr + 1) = powerof2(m)
    the lease significant m-bits of ulPCStartAddr and ulPCEndAddr should all be "0"
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to Set Trax PC stop range.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetPCStop(U32 ulPCStartAddr, U32 ulPCEndAddr, BOOL bInvert)
{
    U8 ucBitIndex;
    U32 ulPCRange;
    U32 ulPCSpanBit;
    U8 ucZeroCnt = 0;
    PCMATCHCTRLREG tPCMatchCtrl;
    TRAXCTRLREG tTraxCtrl;

    if (ulPCEndAddr <= ulPCStartAddr)
    {
        DBG_Getch();
    }
    ulPCRange = ulPCEndAddr - ulPCStartAddr + 1;
    ulPCSpanBit = ulPCRange & (1 + ~ulPCRange);
    if (ulPCRange != ulPCSpanBit)
    {
        DBG_Printf("PC stop range should be power of 2\n");
        DBG_Getch();
    }

    for (ucBitIndex = 0; ucBitIndex < 32; ucBitIndex++)
    {
        if (0 == (ulPCRange & (1 << ucBitIndex)))
        {
            ucZeroCnt++;
        }
        else
        {
            break;
        }
    }
     
    HAL_SetTRIGGERPC(ulPCStartAddr);
    COM_MemZero((U32 *)&tPCMatchCtrl, sizeof(PCMATCHCTRLREG)>>2);
    tPCMatchCtrl.bsPCML = ucZeroCnt & 0x1f; //bsPCML 5-bits
    tPCMatchCtrl.bsPCMS = bInvert;
    HAL_SetPCMATCHCTRL(&tPCMatchCtrl);

    tTraxCtrl = HAL_GetTRAXCTRL();
    tTraxCtrl.bsPCMEN = TRUE;
    HAL_SetTRAXCTRL(&tTraxCtrl);

    return;
}


/*----------------------------------------------------------------------------
Name: HAL_TraxSetPCStop
Description:
    get Trax PC stop range.
Input Param:
    none
Output Param:
    U32 * pPCStartAddr: pointer of PCStartAddr
    U32 * pPCEndAddr: pointer of PCEndAddr
    BOOL * pInvert: pointer of Invert or not
Return Value:
    none
Usage:
    invoke it to get Trax PC stop range.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetPCStop(U32 * pPCStartAddr, U32 * pPCEndAddr, BOOL * pInvert)
{   
    U8 ucPower;
    TRAXCTRLREG tTraxCtrl;
    PCMATCHCTRLREG tPCMatchCtrl;
    BOOL bSuccess;

    tTraxCtrl = HAL_GetTRAXCTRL();
    if (FALSE == tTraxCtrl.bsPCMEN)
    {
        return FALSE;
    }
    
    tPCMatchCtrl = HAL_GetPCMATCHCTRL();
    
    ucPower = tPCMatchCtrl.bsPCML;
    
    *pPCStartAddr = HAL_GetTRIGGERPC();
    *pPCEndAddr = TraxPowerOf2(ucPower) + *pPCStartAddr - 1;
    *pInvert = tPCMatchCtrl.bsPCMS;

    return TRUE;
}

/*----------------------------------------------------------------------------
Name: HalGetTraceSize
Description:
    get Trax supported default trace size.
Input Param:
    none
Output Param:
    TRAX_CONTEXT *pContext: Trax context
Return Value:
    U16: trace size.
Usage:
    invoke it to get Trax supported default trace size.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
LOCAL U16 HalGetTraceSize(TRAX_CONTEXT *pContext)
{
    U16 usStartAddr;
    U16 usEndAddr;
    S16 ssTraceSize;

    usStartAddr = HAL_GetMEMSTARTADDR();
    usEndAddr = HAL_GetMEMENDADDR();
    
    ssTraceSize = usEndAddr - usStartAddr;
    if (ssTraceSize < 0)
    {
      ssTraceSize += ((pContext->ulTraxTramSize >> 2) - 1);
    }
    /* In terms of bytes */
    ssTraceSize = (ssTraceSize + 1) << 2;

    return (U16)ssTraceSize;
}
/*----------------------------------------------------------------------------
Name: HAL_TraxSetPostSize
Description:
    set Trax post size.
Input Param:
    none
Output Param:
    U32 usDelayCount: delay count
    U8 ucUnitType: 0:by word; 1:by instruction; 2:by percent;
Return Value:
    none
Usage:
    invoke it to set Trax post size.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxSetPostSize(TRAX_CONTEXT *pContext, U32 ulDelayCount,  U8 ucUnitType)
{
    U16 usMaxPercent;
    U16 usTraceSize;
    U32 ulDelayUnitCnt;
    TRAXCTRLREG tTraxCtrl;
    
    
    tTraxCtrl = HAL_GetTRAXCTRL();
    switch(ucUnitType)
    {
        case 0:
        {
            tTraxCtrl.bsCNTU = 0;
            if(ulDelayCount >= 0x4000000)
            {
                DBG_Printf("DelayCount by word is too large");
                DBG_Getch();
            }
            ulDelayUnitCnt = ulDelayCount/4;//by DW
            break;
        }
        case 1:
        {
            tTraxCtrl.bsCNTU = 1;
            if (ulDelayCount >=0x01000000)
            {
                DBG_Printf("DelayCount by instruction is too large");
                DBG_Getch();
            }
            ulDelayUnitCnt = ulDelayCount;
            break;
        }
        case 2:
        {
            tTraxCtrl.bsCNTU = 0;
            usTraceSize = HalGetTraceSize(pContext);
            usMaxPercent = (0x4000000/usTraceSize)*100;
            if(ulDelayCount > usMaxPercent)
            {
                DBG_Printf("DelayCount percent is too large");
                DBG_Getch();
            }

            /* convert to number of words. copy form cadence's driver*/
            ulDelayUnitCnt = ((ulDelayCount/100) * usTraceSize)/4; //((usDelayCount * (usTraceSize/16)) /25);
            break;
        }
        default:
        {
            DBG_Getch();
        }
    }

    if (tTraxCtrl.bsCNTU == 0)
    {
        ulDelayUnitCnt = (ulDelayUnitCnt < (POSTSIZE_ADJUST/4)) ? 0 : ulDelayUnitCnt - (POSTSIZE_ADJUST/4);
    }

    HAL_SetTRAXCTRL(&tTraxCtrl);
    HAL_SetDELAYCOUNT(ulDelayUnitCnt);
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxSetPostSize
Description:
    get Trax post size.
Input Param:
    none
Output Param:
    U32 * pDelayCount: pointer of delay count
    U8 * pUnitByDW: 0:by word; 1:by instruction
Return Value:
    none
Usage:
    invoke it to get Trax post size.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_TraxGetPostSize(U32 * pDelayCount, U8 * pUnitType)
{
    U32 ulDelayCnt;
    TRAXCTRLREG tTraxCtrl;
    
    ulDelayCnt = HAL_GetDELAYCOUNT();
    tTraxCtrl = HAL_GetTRAXCTRL();
    
    if(0 == tTraxCtrl.bsCNTU)//by word or percent
    {
        if (ulDelayCnt > 0)
        {
            ulDelayCnt += (POSTSIZE_ADJUST/4);
            *pDelayCount = (int)(ulDelayCnt * 4);
        }
        *pUnitType = 0;
    }
    else//by instruction
    {
        *pDelayCount = ulDelayCnt;
        *pUnitType = 1;
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_TraxReadTraceMem
Description:
    Read Trax data out to a place directed by pData pointer.
Input Param:
    TRAX_CONTEXT * pTraxContext: trax context
    U32 *pData: memory start address which store trax data.
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to read out trax data.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HalTraxReadTraceMem(TRAX_CONTEXT * pTraxContext, U32 *pData)
{
    U16 usMemStartAddr;
    U16 usMemEndAddr;
    U32 ulAddr;
    U32 ulTotalBytes;
    U32 ulLenIndex = 0;
    U32 *pReadData;

    /* To read trace memory by ERI: write the addr into the TRAX_ADDR and then read TRAX_DATA*/
    ulAddr = pTraxContext->usAddrReadLast;
    usMemStartAddr = pTraxContext->usStartAddr;
    usMemEndAddr = pTraxContext->usEndAddr;
    ulTotalBytes = pTraxContext->ulTotalMemlen;
    pReadData = pData;

    while (ulLenIndex < ulTotalBytes)
    {      
        HAL_SetTRAXADDR(ulAddr);
        *pReadData = HAL_GetTRAXDATA();

        pReadData++;
        ulAddr++;
        if (ulAddr > usMemEndAddr)
        {
            ulAddr = usMemStartAddr;
        }
        ulLenIndex = ulLenIndex + 4;
    } 
  
  return;
}

/*----------------------------------------------------------------------------
Name: HalGetTraceInit
Description:
    Init trax context before read out trax data.
Input Param:
    TRAX_CONTEXT *pContext:trax context
Output Param:
    none
Return Value:
    BOOL:TURE:Init success; FALSE:Init fail.
Usage:
    invoke it to Init trax context before read out trax data.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
LOCAL BOOL HalGetTraceInit(TRAX_CONTEXT *pContext)
{
    U32 ulTraxAddr;
    U32 ulMemAddr;
    U32 ulRamSize;
    U32 ulAddrMask;
    U32 ulWrapMsk;
    U16 usStartAddr;
    U16 usEndAddr;
    S32 sMemLen;

    /* Check if its safe to access memory */
    if (TRUE == HAL_TraxIsTraceActive())
    {
        return FALSE;
    }

    /* Get the address register */
    ulTraxAddr = HAL_GetTRAXADDR();

    ulRamSize = pContext->ulTraxTramSize;

    usStartAddr = HAL_GetMEMSTARTADDR();
    usEndAddr = HAL_GetMEMENDADDR();

    /* Determine the memory address from which to read and allocate memory in
    * a buffer using malloc */
    ulAddrMask= (ulRamSize >> 2) - 1;    /* address indexes words */
    ulWrapMsk = ~ulAddrMask;

    /* Figure the wraparound */
    if (ulTraxAddr & ulWrapMsk) 
    {
        ulMemAddr = (ulTraxAddr & ulAddrMask);
        sMemLen = (usEndAddr - usStartAddr + 1) * 4;

        if (sMemLen <= 0)
        {
            sMemLen = sMemLen + ulRamSize;
        }
    }
    else 
    {
        ulMemAddr = usStartAddr;
        sMemLen = ((ulTraxAddr & ulAddrMask)  - usStartAddr) * 4;
        if (sMemLen <= 0)
        {
            sMemLen = sMemLen + ulRamSize;
        }
    }

    /* Maintain state of the address that was last read and
    * the total trace memory to be read */
    pContext->usAddrReadLast = (U16)ulMemAddr;
    pContext->ulTotalMemlen = (U32)sMemLen;
    pContext->usStartAddr = usStartAddr;
    pContext->usEndAddr = usEndAddr;

    return TRUE;
}


/*----------------------------------------------------------------------------
Name: HalGetTraceInit
Description:
    generate trax file header.
Input Param:
    U32 ulSize:trax data size
    U32 ulFlags:ocd enable flag when trax work
    TRAX_FILE_HEAD *pHeaderPtr:trax file header pointer
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to generate trax file header.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
LOCAL void HalTraxGenerateHeader(U32 ulSize, U32 ulFlags, TRAX_FILE_HEAD *pHeaderPtr)
{
    const U32 *pRead;
    U8 *pUsername;
    U8 * pMagic;
    U8 * pToolVer;
    U8 ucIndex;
    TRAX_FILE_HEAD tHeader = *pHeaderPtr;

    /* Generate header */
    COM_MemZero((U32 *)(&tHeader), sizeof(TRAX_FILE_HEAD)>>2);
    pMagic = TRAX_FHEAD_MAGIC;
    COM_MemCpy((U32*)tHeader.aMagic, (U32 *)pMagic, 2);

    tHeader.ucVersion= TRAX_FHEAD_VERSION;
    tHeader.ulTraceOffset= sizeof (tHeader);
    tHeader.ulTraceSize= ulSize;
    tHeader.ulFileSize= sizeof (tHeader) + ulSize;
    tHeader.ulDumpTime= 0;
    tHeader.ulFlags= ulFlags;

    pUsername="(unknown)";
    pToolVer = "toolver";
    COM_MemCpy((U32*)tHeader.ucUserName, (U32 *)pUsername, 4);
    COM_MemCpy((U32*)tHeader.ucToolVer, (U32 *)pToolVer, 2);

    tHeader.ulId = XT_RER(TRAX_ID);
    tHeader.ulControl = XT_RER(TRAX_CTRL);
    tHeader.ulStatus = XT_RER(TRAX_STAT);
    tHeader.ulAddress = XT_RER(TRAX_ADDR);
    tHeader.ulTrigger = XT_RER(TRAX_TRIGGERPC);
    tHeader.ulMatch = XT_RER(TRAX_PCMATCHCTRL);;
    tHeader.ulDelay = XT_RER(TRAX_DELAYCOUNT);

    /* Retrieve some set of registers (including startaddr and endaddr, if defined)*/
    for (pRead = aReadableReg + 7, ucIndex = 0; ucIndex < (TRAX_READABLE_REG_NUM - 7); pRead++, ucIndex++) 
    {
        tHeader.aTraxRegs[ucIndex] = HAL_GetTraxReg(*pRead);
    }

    *pHeaderPtr = tHeader;

    return;
}


/*----------------------------------------------------------------------------
Name: HAL_TraxGetTrace
Description:
    entrance function of get trax data.
Input Param:
    TRAX_CONTEXT * pContext:trax context.
    U8 * pFileBuf:trax file store memory pointer.
Output Param:
    none
Return Value:
    BOOL: TRUE:success; FALSE:fail
Usage:
    invoke it to get trax file(header and data).
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
BOOL HAL_TraxGetTrace(TRAX_CONTEXT * pContext, U8 * pFileBuf)
{
    TRAX_FILE_HEAD tTraxHeader;
    U8 * pFileBufTemp;

    if (FALSE == HalGetTraceInit(pContext))
    {
        return FALSE;
    }
    
    HalTraxGenerateHeader(pContext->ulTotalMemlen, pContext->ulFlags, &tTraxHeader);
    COM_MemCpy((U32 *)pFileBuf, (U32 *)&tTraxHeader, sizeof(TRAX_FILE_HEAD)>>2);

    /* Increment buffer by the size of the header */
    pFileBufTemp = pFileBuf + sizeof(TRAX_FILE_HEAD);

    if (pContext->ulTotalMemlen > 0) 
    { 
        /* Read memory API function takes in the address from which to read,
         * the number of bytes to read and puts the result into a buffer */
        HalTraxReadTraceMem(pContext, (U32 *)pFileBufTemp);
    }

    return TRUE;
}

/*----------------------------------------------------------------------------
Name: HAL_ConfigTraceBuffSingleMcu
Description:
    configure single mcu use trax memory.
Input Param:
    U32 ulMcuID:MCU Id
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to configure single mcu use trax memory.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_ConfigTraceBuffSingleMcu(U32 ulMcuID)
{
    U32 ulTmp = 0;
#ifdef TRACE_MXDSRAM1_3
    if (MCU0_ID == ulMcuID)
    {
        ulTmp |= 3<<8;
        ulTmp |= 0<<10;
        rGlbMCUSramMap |= ulTmp;
    }
    else if (MCU1_ID == ulMcuID)
    {
        ulTmp |= 3<<8;
        ulTmp |= 1<<10;
        rGlbMCUSramMap |= ulTmp;
    }
    else if (MCU2_ID == ulMcuID)
    {
        ulTmp |= 3<<8;
        ulTmp |= 2<<10;
        rGlbMCUSramMap |= ulTmp;
    }
    else
    {
        DBG_Getch();
    }
#else
    if (MCU0_ID == ulMcuID)
    {
        ulTmp |= 3<<3;
        ulTmp |= 0<<6;
        rGlbMCUSramMap |= ulTmp;
    }
    else if (MCU1_ID == ulMcuID)
    {
        ulTmp |= 3<<3;
        ulTmp |= 1<<6;
        rGlbMCUSramMap |= ulTmp;
    }
    else if (MCU2_ID == ulMcuID)
    {
        ulTmp |= 3<<3;
        ulTmp |= 2<<6;
        rGlbMCUSramMap |= ulTmp;
    }
    else
    {
        DBG_Getch();
    }
#endif

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_ConfigTraceBuffSingleMcu
Description:
    configure 3 mcu use trax memory.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to configure 3 mcu use trax memory.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_CofigTraceBuffMutiMcu(void)
{
    U32 ulTmp = 0;
#ifdef TRACE_MXDSRAM1_3
    ulTmp |= 2<<8;
    rGlbMCUSramMap |= ulTmp;
#else

    ulTmp |= 2<<3;
    rGlbMCUSramMap |= ulTmp;
#endif

    return;
}

/*----------------------------------------------------------------------------
Name: HalCalTracBuffRange
Description:
    calculate mcu trace Ram usage range when 3 core all use trax function.
Input Param:
    U32 ulMcuId:mcu id
    U16 usRamSizeByte:trace ram size one mcu use
Output Param:
    U16 * pStartAddr:trace range start addr
    U16 *pEndAddr:trace range end addr
Return Value:
    none
Usage:
    invoke it to calculate mcu trace Ram usage range when 3 core all use trax function.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
LOCAL void HalCalTracBuffRange(U32 ulMcuId, U16 usRamSizeByte, U16 * pStartAddr, U16 *pEndAddr)
{
    U16 usSizDw;

    usSizDw = usRamSizeByte>>2;

    *pStartAddr = (ulMcuId - MCU0_ID)*usSizDw;
    *pEndAddr = (*pStartAddr + usSizDw) - 1;

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_Trax3CoreConfig
Description:
    Configure and start trax module for special mcu. This is 3 mcu's tarx all work mode.
Input Param:
    U32 ulMcuId:mcu id
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it to Configure and start trax module for special mcu.
History:
    20141120    Tobey   create
------------------------------------------------------------------------------*/
void HAL_Trax3CoreConfig(U32 ulMcuId)
{
    U16 usStartAddr;
    U16 usEndAddr;
    U32 ulTraceRamSize;
    U32 ulDelayCount;
    U8 ucUnitType;
    U32 ulPCStartAddr;
    U32 ulPCEndAddr;
    BOOL bInvert;
    U8 * pTraxFileBuff;
    TRAX_CONTEXT tTraxContext;

    ulDelayCount = 0;
    ucUnitType = 0;
    if (MCU0_ID == ulMcuId)
    {
        HAL_CofigTraceBuffMutiMcu();
    }

    HAL_TraxInitERI(&tTraxContext);

    /* logic designed TraceBuff is 8k or 32k */
    ulTraceRamSize = TRACE_RAM_SIZE_MULTICORE;//8<<10;
    HalCalTracBuffRange(ulMcuId, ulTraceRamSize, &usStartAddr, &usEndAddr);

    HAL_TraxSetRamBoundary(&tTraxContext, usStartAddr, usEndAddr);
    usStartAddr = 0xff;
    usEndAddr = 0xff;
    HAL_TraxGetRamBoundary(&usStartAddr, &usEndAddr);
    DBG_Printf("Ram Boundary Start 0x%x; End 0x%x\n", usStartAddr, usEndAddr);

    HAL_TraxSetSMPER(ulTraceRamSize);

    HAL_TraxSetPostSize(&tTraxContext, ulDelayCount, ucUnitType);
    ulDelayCount = 0;
    ucUnitType = 0xff;
    HAL_TraxGetPostSize(&ulDelayCount, &ucUnitType);
    DBG_Printf("PostSize DelayCount 0x%x; ucUnitType 0x%x\n", ulDelayCount, ucUnitType);

    HAL_TraxSetPTIStop();
    if (FALSE == HAL_TraxGetPTIStop())
    {
        DBG_Printf("PTIEN setting error!!\n");
    }

    HAL_TraxStart();

    return;
}

#endif//#if defined(FPGA)  || defined(ASIC) || defined(COSIM)

