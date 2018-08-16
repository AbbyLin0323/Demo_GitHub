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
Filename     : HAL_SGE.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Gavin

Description: 
     Scatter gather engine,we named it as SGE,is a module to indicate the 
     host/device memory assignments and to control the data transferring 
     between the host/device memory.
     For VT3514B0 ASIC ,the SGE supports DRQ for completing the data stream 
     from device(DRAM) to host ,and DWQ for host to device(DRAM).And SGQ for 
     OTFB to host memory. 
Modification History:
20130909    Gavin   created
20140915    Victor  modified according to new coding style 
*******************************************************************************/


#include "BaseDef.h"
#include "Proj_Config.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_SGE.h"
#include "HAL_HwDebug.h"
#include "HAL_MultiCore.h"
#include "HAL_GLBReg.h"

#ifdef SIM
#include "sim_SGE.h"
#endif

LOCAL MCU12_VAR_ATTR volatile DRQ_DWQ_REG *l_pDrqStatus;
LOCAL MCU12_VAR_ATTR volatile DRQ_DWQ_REG *l_pDwqStatus;
LOCAL MCU12_VAR_ATTR volatile U32 *l_pSgqStatus;
LOCAL MCU12_VAR_ATTR U32 l_ulDrqBaseAddr;
LOCAL MCU12_VAR_ATTR U32 l_ulDwqBaseAddr;

LOCAL volatile CHAIN_NUM_REG *l_pChainNumReg;

//used when need to set chain number to 0 for peer subsystem
LOCAL volatile CHAIN_NUM_REG *l_pPeerChainNumReg; 


/*========================================================================
    PATCH FOR SPECIAL ENV
========================================================================*/

#ifdef SIM
/* 
    Under ASIC/FPGA env, when driver updates the chain number register ,the SGE 
    module should response the update immediately,but under windows simulation ,
    we simulate the SGE module as a thread, for ensuring the SGE thread responsed 
    in time , driver should call the SGE_CheckChainNumComplete directly here.

    And the 'g_CriticalChainNumFw' was used for avoiding multiple threads collision ,
    as the interface of chain number we update is called by SGE/MCU1/MCU2/NFC 
    threads.
*/
extern CRITICAL_SECTION  g_CriticalChainNumFw;
extern void SGE_CheckChainNumComplete(U8 ucTag);
#define GET_CHAIN_NUM_LOCK EnterCriticalSection(&g_CriticalChainNumFw)
#define RELEASE_CHAIN_NUM_LOCK LeaveCriticalSection(&g_CriticalChainNumFw)
#else

/*
    For patching the A0 hardware bug, for later version it will be ignored .
*/
#ifdef A0_HW_BUG_PATCH
#define GET_CHAIN_NUM_LOCK        SgeGetLock()
#define RELEASE_CHAIN_NUM_LOCK    SgeReleaseLock()
#else
#define GET_CHAIN_NUM_LOCK 
#define RELEASE_CHAIN_NUM_LOCK 
#endif
#endif

/*========================================================================
    EXTERNAL INTERFACE
========================================================================*/
extern U32 HAL_GetMcuId();

/*========================================================================
    PATCH FUNCTION
========================================================================*/

/* Patch hardware bug on VT3514_A0 */
#ifdef A0_HW_BUG_PATCH
//software lock for patch HW bug in VT3514_A0
LOCAL void SgeGetLock(void)
{
    while( FALSE == HAL_MultiCoreGetSpinLock(SPINLOCKID_SUBSYS_SGE) )
    {
        //spin lock is hoding by other MCU,wait
        ;
    }
}

LOCAL void SgeReleaseLock(void)
{
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_SUBSYS_SGE);
}

LOCAL void SgePatchHwBug(U8 ucHID)
{
    CHAIN_NUM_REG tChainNumReg;
    tChainNumReg.ulChainNumReg = 0;
    tChainNumReg.bsHID = ucHID;

    rChainNumMcu1 = tChainNumReg.ulChainNumReg;
}
#endif 

/*========================================================================
    FUNCTIONS IMPLEMENTING
========================================================================*/

/*========================================================================
Function :HAL_DrqInit
Input : void
Output : void
Description:
    Choose the appropriate bank of base address register and drq status register 
    according to current mcu.   
History:
    20140915   Victor Zhang  Create
========================================================================*/

void HAL_DrqInit(void)
{
    if(MCU0_ID == HAL_GetMcuId())
    {
        l_pDrqStatus    = (DRQ_DWQ_REG*)&(rDrqMcu0Status);
        l_ulDrqBaseAddr = DRQ_BASE_ADDR_MCU0;
        COM_MemZero((U32 *)l_ulDrqBaseAddr,DRQ_DEPTH_MCU0);
    }
    else if(MCU1_ID == HAL_GetMcuId())
    {
        l_pDrqStatus    = (DRQ_DWQ_REG*)&(rDrqMcu1Status);
        l_ulDrqBaseAddr = DRQ_BASE_ADDR_MCU1;
        COM_MemZero((U32 *)l_ulDrqBaseAddr,DRQ_DEPTH_MCU1);
    }
    else if(MCU2_ID == HAL_GetMcuId())
    {
        l_pDrqStatus    = (DRQ_DWQ_REG*)&(rDrqMcu2Status);
        l_ulDrqBaseAddr = DRQ_BASE_ADDR_MCU2;
        COM_MemZero((U32 *)l_ulDrqBaseAddr,DRQ_DEPTH_MCU2);
    }
    else
    {
        DBG_Getch();
    }
}

/*========================================================================
Function :HAL_DrqIsFull
Input : void
Output : BOOL  TRUE for Full ; FALSE for Not full
Description:
    Get the drq full status by accessing drq status register.    
History:
    20140915   Victor Zhang  Create
========================================================================*/

BOOL HAL_DrqIsFull(void)
{
    return (BOOL)(l_pDrqStatus->bsFull);
}

/*========================================================================
Function :HAL_DrqIsEmpty
Input : void
Output : BOOL  TRUE for Empty ; FALSE for Not empty
Description:
    Get the drq empty status by accessing drq status register. 
History:
    20140915   Victor Zhang  Create    
========================================================================*/

BOOL HAL_DrqIsEmpty(void)
{
    return (BOOL)(l_pDrqStatus->bsEmpty);
}

/*========================================================================
Function :HAL_TrigDrq
Input :  void
Output : void
Description:
    Under windows simulation,driver will move the drq write pointer ahead directly.
    Under other env,driver will trigger SGE by set the bsTrig bit.
History:
    20140915   Victor Zhang  Create    
========================================================================*/

void HAL_TrigDrq(void)
{
    U8 ucGroupSel;
#ifdef SIM
    ucGroupSel = HAL_GetMcuId() - MCU0_ID; 
    SGE_DrqMoveWritePtr(ucGroupSel);
    SGE_ModelSchedule();
#else
    l_pDrqStatus->bsTrig = TRUE;
#endif
}

/*========================================================================
Function :HAL_DrqBuildEntry
Input : void
Output : BOOL  TRUE for Generate successfully ,FALSE for failed (queue is full)
Description:
    Generate a drq entry.   
    1. If the drq is not full ,a new entry will be generated and pushed into 
       the queue ; full nothing be done.
    2. Get a drq entry according to the drq base address and write pointer.
    3. Set the entry and trig.
History:
    20140915   Victor Zhang  Create
========================================================================*/

BOOL HAL_DrqBuildEntry(U8 ucHID, U16 usFirstHsgId, U16 usFirstDsgId)
{
    SGE_ENTRY * pDrqEntry ;   
    if(TRUE == HAL_DrqIsFull())
    {
        //ASSERT( FALSE );
        return FALSE;
    }

    pDrqEntry = (SGE_ENTRY*)(l_ulDrqBaseAddr + (l_pDrqStatus->bsWtPtr * sizeof(SGE_ENTRY)));
    COM_MemZero((U32*)pDrqEntry,sizeof(SGE_ENTRY)/sizeof(U32));

    pDrqEntry->bsHID = ucHID;
    pDrqEntry->bsHsgPtr = usFirstHsgId;
    pDrqEntry->bsDsgPtr = usFirstDsgId;
    pDrqEntry->bsDChainInvalid = FALSE;
    HAL_TrigDrq();

    return TRUE;
}


/* DWQ interface */

/*========================================================================
Function :HAL_DwqInit
Input : void
Output : void
Description:
    Choose the appropriate bank of base address register 
    and dwq status register according to current mcu.    
History:
    20140915   Victor Zhang  Create
========================================================================*/

void HAL_DwqInit(void)
{
    if(MCU0_ID == HAL_GetMcuId())
    {
        l_pDwqStatus    = (DRQ_DWQ_REG*)&(rDwqMcu0Status); 
        l_ulDwqBaseAddr = DWQ_BASE_ADDR_MCU0; 
        COM_MemZero((U32 *)l_ulDwqBaseAddr,DWQ_DEPTH_MCU0);
    }
    else if(MCU1_ID == HAL_GetMcuId())
    {
        l_pDwqStatus    = (DRQ_DWQ_REG*)&(rDwqMcu1Status); 
        l_ulDwqBaseAddr = DWQ_BASE_ADDR_MCU1; 
        COM_MemZero((U32 *)l_ulDwqBaseAddr,DWQ_DEPTH_MCU1);
    }
    else if(MCU2_ID == HAL_GetMcuId())
    {
        l_pDwqStatus    = (DRQ_DWQ_REG*)&(rDwqMcu2Status);
        l_ulDwqBaseAddr = DWQ_BASE_ADDR_MCU2;
        COM_MemZero((U32 *)l_ulDwqBaseAddr,DWQ_DEPTH_MCU2);
    }
    else
    {
        DBG_Getch();
    }

}

/*========================================================================
Function :HAL_DwqIsFull
Input : void
Output : BOOL  TRUE for Full ; FALSE for Not full
Description:
    Get the dwq full status by accessing dwq status register.    
History:
    20140915   Victor Zhang  Create
========================================================================*/


BOOL HAL_DwqIsFull(void)
{
    return (BOOL)(l_pDwqStatus->bsFull);
}

/*========================================================================
Function :HAL_GetDwqStatus
Input : void
Output : DRQ_DWQ_REG*  Pointer to current status register
Description:
    Get the value of dwq status register.    
History:
    20140915   Victor Zhang  Create
========================================================================*/

BOOL HAL_DwqIsEmpty(void)
{
    return (BOOL)(l_pDwqStatus->bsEmpty);
}

/*========================================================================
Function :HAL_TrigDwq
Input :  void
Output : void
Description:
    Under windows simulation,driver will move the dwq write pointer ahead directly.
    Under other env,driver will trigger SGE by set the bsTrig bit.
History:
    20140915   Victor Zhang  Create    
========================================================================*/

void HAL_TrigDwq(void)
{
    U8 ucGroupSel;
#ifdef SIM
    ucGroupSel = HAL_GetMcuId() - MCU0_ID; 
    SGE_DwqMoveWritePtr(ucGroupSel);
    SGE_ModelSchedule();
#else
    l_pDwqStatus->bsTrig = TRUE;
#endif
}

/*========================================================================
Function :HAL_DwqBuildEntry
Input : void
Output : BOOL  TRUE for Generate successfully ,FALSE for failed (queue is full)
Description:
    Generate a dwq entry.   
    1. If the dwq is not full ,a new entry will be generated and pushed into 
       the queue ; full nothing be done.
    2. Get a dwq entry according to the dwq base address and write pointer.
    3. Set the entry and trig.
History:
    20140915   Victor Zhang  Create
========================================================================*/

BOOL HAL_DwqBuildEntry(U8 ucHID, U16 usFirstHsgId, U16 usFirstDsgId)
{
    SGE_ENTRY * pDwqEntry;
    
    if(TRUE == HAL_DwqIsFull())
    {
        //ASSERT( FALSE );
        return FALSE;
    }

    pDwqEntry = (SGE_ENTRY*)(l_ulDwqBaseAddr + (l_pDwqStatus->bsWtPtr * sizeof(SGE_ENTRY)));
    COM_MemZero((U32*)pDwqEntry,sizeof(SGE_ENTRY)/sizeof(U32));
    
    pDwqEntry->bsHID = ucHID;
    pDwqEntry->bsHsgPtr = usFirstHsgId;
    pDwqEntry->bsDsgPtr = usFirstDsgId;
    pDwqEntry->bsDChainInvalid = FALSE;

    //Review if the HAL_HwDebugTrace() is necessary and its consume cycle counts.
    // l_bHwDebugTraceEn = (pPTable->tHALFeature.ulFeatureBitMap & 0x1) ? TRUE : FALSE;
    //(-28 Cycle)
#if 0
    HAL_HwDebugTrace(ucHID, RCD_DWQ, pDwqEntry, l_pDwqStatus->bsWtPtr, NULL);
#endif
    HAL_TrigDwq();

    return TRUE;
}

/* SGQ interface */
U32 HAL_GetSgqEntryId(U8 ucPhyPu ,U8 ucLun, U8 ucLev)
{
    return ucPhyPu * NFCQ_DEPTH_TOTAL +  ucLun* NFCQ_DEPTH + ucLev;
}
/*========================================================================
Function :HAL_SgqIsBusy
Input :  PU -- Pu 
         Level -- NFC write pointer
Output : void
Description:
    Check SGQ status ,if the related data transferring was processing ,the SGQ entry 
    would be busy ,then fw should not set the SGQ entry until the busy bit was cleared.
History:
    20140915   Victor Zhang  Create 
    20151028    abby         add para ucLun
    20160329    Janka        Modify for sgq expansion from 128 to 256
========================================================================*/
BOOL HAL_SgqIsBusy(U8 ucPU, U8 ucLun)
{    
    U8 ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    U8 ucLevel = HAL_NfcGetWP(ucPU, ucLun);
    U32 ulSgqStatus;

    U32  ucSgqStaRegNum;
    U32  ucSgqStaPuMapOffset;
    U32  ucSgqStaPuLevelMapBit;

    ucSgqStaRegNum = ucPhyPU / PUNUM_PER_SGQ_STATUS_REG;
    ucSgqStaPuMapOffset = ucPhyPU % PUNUM_PER_SGQ_STATUS_REG;
    ucSgqStaPuLevelMapBit =  1 <<((ucSgqStaPuMapOffset * NFCQ_DEPTH_TOTAL) + ucLun* NFCQ_DEPTH + ucLevel);

    switch  (ucSgqStaRegNum)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            ulSgqStatus = *(((volatile U32*)rSgqStatusP1Addr) + ucSgqStaRegNum);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            ulSgqStatus = *(((volatile U32*)rSgqStatusP2Addr) + (ucSgqStaRegNum - 4));
            break;
        default:
            DBG_Printf("ucPhyPu is invalid\n", ucPhyPU);
            DBG_Getch();
            break;
    }

    if (0 != (rGlbOTFBMCtrl0 & (1 << 18)))
    {
        DBG_Printf("PU=%d GLB_28\n", ucPU);
        return FALSE;
    }
    
    return (BOOL)(ulSgqStatus & ucSgqStaPuLevelMapBit) ? TRUE : FALSE;
}


/*========================================================================
Function :HAL_GetSgqEntry
Input :  ucPhyPU-- CE 
         Level -- NFC write pointer
Output : void
Description:
    Get the SGQ entry.
History:
    20140915   Victor Zhang  Create    
========================================================================*/
SGE_ENTRY* HAL_GetSgqEntry(U8 ucPhyPu ,U8 ucLun, U8 ucLev)
{
    return (SGE_ENTRY*)(SGQ_BASE_ADDR + HAL_GetSgqEntryId(ucPhyPu, ucLun, ucLev) * sizeof(SGE_ENTRY));
}

/*========================================================================
Function :HAL_SgqBuildEntry
Input :  HID -- command tag 
         FirstHsgId -- First HSG entry ID of the HSG chain
         bWriteEn -- Set for write ,clear for read 
Output : void
Description:
    Set related SGQ entry for host read/write 
History:
    20140915   Victor Zhang  Create    
    20151028    abby         add para ucLun
========================================================================*/

BOOL HAL_SgqBuildEntry(U8 ucHID, U16 usFirstHsgId, U8 ucPU, U8 ucLun, BOOL bWriteEn)
{
    U8 ucPhyPU;
    U8 ucWp;
    SGE_ENTRY * pSgqEntry;
    
    ucPhyPU = HAL_NfcGetPhyPU(ucPU);
    ucWp = HAL_NfcGetWP(ucPU, ucLun);

    pSgqEntry = HAL_GetSgqEntry(ucPhyPU,ucLun,ucWp);
    COM_MemZero((U32 *)pSgqEntry, sizeof(SGE_ENTRY)/sizeof(U32));
    pSgqEntry->bsHID = ucHID;
    pSgqEntry->bsHsgPtr = usFirstHsgId;
    pSgqEntry->bsWriteEn = bWriteEn;

    HAL_HwDebugTrace(ucHID, (TRUE == bWriteEn) ? RCD_SGQ_W : RCD_SGQ_R, pSgqEntry
                   ,(ucPhyPU << 1) + ucWp, g_pNfcqForHalDebug);
    
    return TRUE;
}

/*========================================================================
Function :HAL_SgeInitChainCnt
Input :  void 
Output : void
Description:
    Init the sge chain number register 
History:
    20140915   Victor Zhang  Create    
========================================================================*/

void HAL_SgeInitChainCnt(void)
{
    if(MCU1_ID == HAL_GetMcuId())
    {
        l_pChainNumReg     = (CHAIN_NUM_REG*)&rChainNumMcu1;
        l_pPeerChainNumReg = (CHAIN_NUM_REG*)&rChainNumMcu2;
    }
    else if(MCU2_ID == HAL_GetMcuId())
    {
        l_pChainNumReg     = (CHAIN_NUM_REG*)&rChainNumMcu2;
        l_pPeerChainNumReg = (CHAIN_NUM_REG*)&rChainNumMcu1;
    }
    else
    {
        DBG_Getch();
    }
}

/*========================================================================
Function :HAL_SgeFinishChainCnt
Input :  HID -- command tag
         TotalChain -- total chain number for current sub request on single MCU. 
Output : void
Description:
    Update the chain number register .
History:
    20140915   Victor Zhang  Create    
========================================================================*/

void HAL_SgeFinishChainCnt(U8 ucHID, U16 usTotalChain)
{
    rChainNumMcu1 = 0x01 | ((U32)ucHID << 2) | ((U32)usTotalChain << 8);
    rChainNumMcu2 = 0x01 | ((U32)ucHID << 2);
#ifdef SIM
    SGE_RecordChainNumForMCU1(ucHID, usTotalChain, FALSE);
    SGE_RecordChainNumForMCU2(ucHID, 0, FALSE);
#endif
}

#if 0
//used when need to set chain number to 0 for peer subsystem
/*========================================================================
Function :HAL_SgeHelpFinishChainCnt
Input :  HID -- command tag 
Output : void
Description:
    When we drive the SGE model on single MCU mode , we should set the chain number 
    register of peer MCU (such as we run the program on MCU1 ,the peer MCU 
    would be MCU2 ,and if on MCU2,the peer would be MCU1) by hand.
History:
    20140915   Victor Zhang  Create    
========================================================================*/

void HAL_SgeHelpFinishChainCnt(U8 ucHID)
{
    CHAIN_NUM_REG tChainNumReg;    
    tChainNumReg.ulChainNumReg = 0;
    tChainNumReg.bsHID = ucHID;
    tChainNumReg.bsValid = TRUE;
    GET_CHAIN_NUM_LOCK;
#ifdef A0_HW_BUG_PATCH
    if ((U32)&rChainNumMcu2 == (U32)l_pPeerChainNumReg)
    {
        SgePatchHwBug(ucHID);
    }
#endif
    l_pPeerChainNumReg->ulChainNumReg = tChainNumReg.ulChainNumReg; 
#ifdef SIM
    if (MCU2_ID == HAL_GetMcuId())
    {
        SGE_RecordChainNumForMCU1(ucHID, 0, FALSE);
    }
    else
    {
        SGE_RecordChainNumForMCU2(ucHID, 0, FALSE);
    }
    SGE_CheckChainNumComplete(ucHID); 
#endif
    RELEASE_CHAIN_NUM_LOCK;
}
#endif

/*========================================================================
Function :HAL_SgeStopChainCnt
Input :  HID -- command tag 
Output : void
Description:
    Clear the chain number register for error handling .
History:
    20140915   Victor Zhang  Create    
========================================================================*/

void HAL_SgeStopChainCnt(U8 ucHID)
{
    GET_CHAIN_NUM_LOCK;
    l_pChainNumReg->bsHID = ucHID;
    l_pChainNumReg->bsClear = TRUE;
    RELEASE_CHAIN_NUM_LOCK;
}

/*========================================================================
Function :HAL_SgeResetChainCnt
Input :  None
Output : void
Description:
    Clear all chain number registers for error handling .
History:
    20151214   Yao Chen added
========================================================================*/
void HAL_SgeResetChainCnt(void)
{
    rGlbMcuSgeRst |= R_RST_SGECHNCNT;
    HAL_DelayCycle(20);  //wait 20 clock
    rGlbMcuSgeRst &= ~R_RST_SGECHNCNT;

    return;
}

/*========================================================================
Function :HAL_SgeGetAllEngIdle
Input :  
Output : TRUE --  All eng is idle 
         FALSE -- There is eng trans ongoing 
Description:
    Get the 'All eng idle bit' of MODE & CLKEN register 
History:
    20141119   Victor Zhang  Create    
========================================================================*/
GLOBAL BOOL HAL_SgeGetAllEngIdle(void)
{
    SGE_MODE_CLKEN_REG tReg;
    tReg.ulValue = rModeClkEn;
    return (BOOL)(tReg.bsAllEngIdle);
}

/*========================================================================
Function :HAL_SgeGetAllTransFsh
Input :  
Output : TRUE -- There is no trans pending in HC from SGE 
         FALSE -- There is trans send to HC not complete 
Description:
    Get the 'All trans finish bit' of MODE & CLKEN register 
History:
    20141119 Victor Zhang  Create    
========================================================================*/
GLOBAL BOOL HAL_SgeGetAllTransFsh(void)
{
    SGE_MODE_CLKEN_REG tReg;
    tReg.ulValue = rModeClkEn;
    return (BOOL)(tReg.bsAllTransFsh);
}

/*========================================================================
Function :HAL_IsSGEIdle
Input :  
Output : TRUE -- SGE is total idle
         FALSE -- SGE is not total idle
Description:
    Get the 'All trans finish bit' and 'All eng idle bit' of MODE & CLKEN register 
History:
    20151214 Yao Chen added    
========================================================================*/
GLOBAL BOOL HAL_IsSGEIdle(void)
{
    SGE_MODE_CLKEN_REG tReg;
    tReg.ulValue = rModeClkEn;

    return (BOOL)(tReg.bsAllTransFsh & tReg.bsAllEngIdle);
}

/*========================================================================
Function :HAL_SGEReset
Input :  None
Output : None
Description:
    Resets SGE modules for error handling.
History:
    20151214 Yao Chen added
========================================================================*/
GLOBAL void HAL_SGEReset(void)
{
    rGlbSoftRst |= R_RST_SGE;
    rGlbMcuSgeRst |= (R_RST_SGEHSG | R_RST_SGEDSG | R_RST_SGECFG | R_RST_SGECHNCNT);
    HAL_DelayCycle(20);  //wait 20 clock
    rGlbSoftRst &= ~R_RST_SGE;
    rGlbMcuSgeRst &= ~(R_RST_SGEHSG | R_RST_SGEDSG | R_RST_SGECFG | R_RST_SGECHNCNT);

    return;
}

/*========================================================================
Function :HAL_SGEOTFBMapReset
Input :  None
Output : None
Description:
    Resets OTFB map for error handling.
History:
    20151214 Yao Chen added
========================================================================*/
GLOBAL void HAL_SGEOTFBMapReset(void)
{
    rGlbSoftRst |= R_RST_OTFBM;
    HAL_DelayCycle(20);
    rGlbSoftRst &= ~R_RST_OTFBM;

    return;
}

/*******************************FILE END**************************************/

