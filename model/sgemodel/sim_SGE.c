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

Filename     : sim_HSG.c
Version      :   Ver 1.0
Date         :
Author       :  Gavin

Description:
                HSG model for XTMP/Win simulation
Modification History:
20130902    Gavin   created
*******************************************************************************/

#include "Proj_Config.h"
#include "HAL_HSG.h"
#include "sim_HSG.h"
#include "HAL_MemoryMap.h"
#include "HAL_NormalDSG.h"
#include "HAL_FlashDriverBasic.h"
#include "sim_SGE.h"
#include "sim_NormalDSG.h"
#include "HAL_SGE.h"
#include "sim_flash_common.h"
#include "memory_access.h"
#include "system_statistic.h"
#include "sim_flash_config.h"

#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#include "model_common.h"
#include "iss/mp.h"
#endif

extern U8 g_uCMDFinishFlag[];

extern void Comm_ReadOtfb(U32 addr ,U32 nWords ,U32 *buf);
extern void Comm_WriteOtfb(U32 addr ,U32 nWords ,U32 *buf);
extern void Host_WriteToOTFB(U32 HostAddrHigh ,U32 HostAddrLow ,U32 OTFBAddr ,U32 ByteLen ,U8 nTag);
extern void Host_WriteToDram(U32 HostAddrHigh ,U32 HostAddrLow ,U32 DramAddr ,U32 ByteLen ,U8 nTag);
extern void Host_ReadFromOTFB(U32 HostAddrHigh ,U32 HostAddrLow ,U32 OTFBAddr ,U32 ByteLen ,U8 nTag);
extern void Host_ReadFromDram(U32 HostAddrHigh ,U32 HostAddrLow ,U32 DramAddr ,U32 ByteLen ,U8 nTag);
extern void SIM_HostCWBQDataDoneTrigger(U8);

void SGE_ModelProcess(void);

/* define max command number */

LOCAL BOOL l_bSGEThreadExit = FALSE;
// indicate SGE thread to process DWQ/DRQ/SGE
GLOBAL HANDLE g_SimSgeEvent;
GLOBAL HANDLE g_hSgeThread;

/* local variable */
LOCAL volatile CHAIN_NUM_RECORD l_aChainNumRecord[HID_TOTAL] = {0};

LOCAL U32 l_ulCurDrqTotalLen = 0;
LOCAL U32 l_ulCurDwqTotalLen = 0;


LOCAL U32 l_ulDrqDepth[GROUP_NUM];
LOCAL U32 l_ulDwqDepth[GROUP_NUM];
LOCAL U32 l_aDrqStsRegAddr[GROUP_NUM];
LOCAL U32 l_aDwqStsRegAddr[GROUP_NUM];
LOCAL U32 l_aDrqBaseAddr[GROUP_NUM];
LOCAL U32 l_aDwqBaseAddr[GROUP_NUM];

void SGE_LocalParamInit()
{

    l_ulDrqDepth[0] = DRQ_DEPTH_MCU0;
    l_ulDrqDepth[1] = DRQ_DEPTH_MCU1;
    l_ulDrqDepth[2] = DRQ_DEPTH_MCU2;

    l_ulDwqDepth[0] = DWQ_DEPTH_MCU0;
    l_ulDwqDepth[1] = DWQ_DEPTH_MCU1;
    l_ulDwqDepth[2] = DWQ_DEPTH_MCU2;

    l_aDrqStsRegAddr[0] = DRQ_MCU0_STS_REG_ADDR;
    l_aDrqStsRegAddr[1] = DRQ_MCU1_STS_REG_ADDR;
    l_aDrqStsRegAddr[2] = DRQ_MCU2_STS_REG_ADDR;

    l_aDwqStsRegAddr[0] = DWQ_MCU0_STS_REG_ADDR;
    l_aDwqStsRegAddr[1] = DWQ_MCU1_STS_REG_ADDR;
    l_aDwqStsRegAddr[2] = DWQ_MCU2_STS_REG_ADDR;

    l_aDrqBaseAddr[0] = DRQ_BASE_ADDR_MCU0;
    l_aDrqBaseAddr[1] = DRQ_BASE_ADDR_MCU1;
    l_aDrqBaseAddr[2] = DRQ_BASE_ADDR_MCU2;

    l_aDwqBaseAddr[0] = DWQ_BASE_ADDR_MCU0;
    l_aDwqBaseAddr[1] = DWQ_BASE_ADDR_MCU1;
    l_aDwqBaseAddr[2] = DWQ_BASE_ADDR_MCU2;


#if 0
    DBG_Printf("l_ulDrqDepth[0] = 0x%x \n",DRQ_DEPTH_MCU0);
    DBG_Printf("l_ulDrqDepth[1] = 0x%x \n",DRQ_DEPTH_MCU1);
    DBG_Printf("l_ulDrqDepth[2] = 0x%x \n",DRQ_DEPTH_MCU2);

    DBG_Printf("l_ulDwqDepth[0] = 0x%x \n",DWQ_DEPTH_MCU0);
    DBG_Printf("l_ulDwqDepth[1] = 0x%x \n",DWQ_DEPTH_MCU1);
    DBG_Printf("l_ulDwqDepth[2] = 0x%x \n",DWQ_DEPTH_MCU2);

    DBG_Printf("l_aDrqStsRegAddr[0] = 0x%x \n",DRQ_MCU0_STS_REG_ADDR);
    DBG_Printf("l_aDrqStsRegAddr[1] = 0x%x \n",DRQ_MCU1_STS_REG_ADDR);
    DBG_Printf("l_aDrqStsRegAddr[2] = 0x%x \n",DRQ_MCU2_STS_REG_ADDR);

    DBG_Printf("l_aDwqStsRegAddr[0] = 0x%x \n",DWQ_MCU0_STS_REG_ADDR);
    DBG_Printf("l_aDwqStsRegAddr[1] = 0x%x \n",DWQ_MCU1_STS_REG_ADDR);
    DBG_Printf("l_aDwqStsRegAddr[2] = 0x%x \n",DWQ_MCU2_STS_REG_ADDR);

    DBG_Printf("l_aDrqBaseAddr[0] = 0x%x \n",DRQ_BASE_ADDR_MCU0);
    DBG_Printf("l_aDrqBaseAddr[1] = 0x%x \n",DRQ_BASE_ADDR_MCU1);
    DBG_Printf("l_aDrqBaseAddr[2] = 0x%x \n",DRQ_BASE_ADDR_MCU2);

    DBG_Printf("l_aDwqBaseAddr[0] = 0x%x \n",DWQ_BASE_ADDR_MCU0);
    DBG_Printf("l_aDwqBaseAddr[1] = 0x%x \n",DWQ_BASE_ADDR_MCU1);
    DBG_Printf("l_aDwqBaseAddr[2] = 0x%x \n",DWQ_BASE_ADDR_MCU2);
#endif


}
#ifndef IGNORE_PERFORMANCE
LOCAL ST_TIME_RECORD l_tSgeDrqTimeRecord[DRQ_DWQ_DROUP];
LOCAL ST_TIME_RECORD l_tSgeDwqTimeRecord[DRQ_DWQ_DROUP];
LOCAL U8 l_ucSgeDrqPcieSts[DRQ_DWQ_DROUP];
LOCAL U8 l_ucSgeDwqPcieSts[DRQ_DWQ_DROUP];
GLOBAL U8 l_ucPcieBusHostToDevSts = PCIE_STS_FREE;
GLOBAL U8 l_ucPcieBusDevToHostSts = PCIE_STS_FREE;
GLOBAL CRITICAL_SECTION g_PcieDevToHostCriticalSection;
GLOBAL CRITICAL_SECTION g_PcieHostToDevCriticalSection;
#endif

CRITICAL_SECTION l_CriticalDrq[GROUP_NUM];
CRITICAL_SECTION l_CriticalDwq[GROUP_NUM];
CRITICAL_SECTION l_CriticalChainNumAdd;
CRITICAL_SECTION l_CriticalChainNumCheck;
CRITICAL_SECTION g_CriticalChainNumFw;

/******************************************************
FUN: SGE_WriteStatusReg
INPUT: ulGrpSel -- 0 : MCU0 (VT3514_C0) / MCU1(VT3514_B0)
       ulValue  -- input register value
       bIsDrq   -- TRUE for DRQ ,FALSE for DWQ
DES:
    Write the DRQ/DWQ status register  of MCU 0/1/2

******************************************************/

LOCAL void SGE_WriteStatusReg(U32 ulGrpSel,U32 ulValue,BOOL bIsDrq)
{
    U32 *pReg = (TRUE == bIsDrq) ? (l_aDrqStsRegAddr) : (l_aDwqStsRegAddr);
#ifdef SIM
    *(U32*)(pReg[ulGrpSel]) = ulValue;
#else
    U32 ulBuf = ulValue;
    Comm_WriteReg(pReg[ulGrpSel],1,&ulBuf);
#endif
}

/******************************************************
FUN: SGE_ReadStatusReg
RET  : register value
INPUT: ulGrpSel -- 0 : MCU0 (VT3514_C0) / MCU1(VT3514_B0)
       bIsDrq   -- TRUE for DRQ ,FALSE for DWQ
DES:
    Read the DRQ/DWQ status register of MCU 0/1/2

******************************************************/
LOCAL U32 SGE_ReadStatusReg(U32 ulGrpSel,BOOL bIsDrq)
{
    U32 *pReg = (TRUE == bIsDrq) ? (l_aDrqStsRegAddr) : (l_aDwqStsRegAddr);
#ifdef SIM
    return *(U32*)(pReg[ulGrpSel]);
#else
    U32 ulBuf;
    Comm_ReadReg(pReg[ulGrpSel],1,&ulBuf);
    return ulBuf;
#endif
}
/******************************************************
FUN: SGE_WriteChainNumReg
RET  : void
INPUT: ulValue  -- input register value
       bIsMcu1  -- TRUE for MCU1 chain number reg
                   FALSE for MCU2 chain number reg
DES:
    Write MCU1/MCU2 chain number reg

NOTE :
    MCU0 does not record chain number.

******************************************************/
LOCAL void SGE_WriteChainNumReg(U32 ulValue,BOOL bIsMcu1)
{
    U32 ulRegAddr = (TRUE == bIsMcu1)?(CHAIN_NUM1_REG_ADDR):(CHAIN_NUM2_REG_ADDR);
#ifdef SIM
    *(U32*)ulRegAddr = ulValue;
#else
    U32 ulBuf = ulValue;
    Comm_WriteReg(ulRegAddr,1,&ulBuf);
#endif
}
/******************************************************
FUN: SGE_RecordChainNumForMCU1(U8 Tag,U16 ChainNum, BOOL bNfcChain)
INPUT: Tag --- Current host cmd id
       ChainNum --- the total chain number of MCU 1
       bNfcChain --- is the NFC on-the-fly program chain number
DES:
    Update the local chain number record struct at the moment of
    the valid bit of rChainNumMcu1 be set.Details as follow:
    1.Lock the l_aChainNumRecord
    2.Record the chain number and vaild by tag
    3.Clear rChainNumMcu1

******************************************************/
void SGE_RecordChainNumForMCU1(U8 Tag, U16 ChainNum, BOOL bNfcChain)
{
    EnterCriticalSection(&l_CriticalChainNumAdd);
    l_aChainNumRecord[Tag].ChainNum1 = ChainNum;
    l_aChainNumRecord[Tag].Mcu1Valid = TRUE;
    SGE_WriteChainNumReg(0, TRUE);

    LeaveCriticalSection(&l_CriticalChainNumAdd);
}
/******************************************************
FUN: SGE_RecordChainNumForMCU2
INPUT: Tag --- Current host cmd id
       ChainNum --- the total chain number of MCU 2
       bNfcChain --- is the NFC on-the-fly program chain number
DES:
    Update the local chain number record struct at the moment of
    the valid bit of rChainNumMcu1 be set.Details as follow:
    1.Lock the l_aChainNumRecord
    2.Record the chain number and vaild by tag
    3.Clear rChainNumMcu1

******************************************************/
void SGE_RecordChainNumForMCU2(U8 Tag ,U16 ChainNum, BOOL bNfcChain)
{
    EnterCriticalSection(&l_CriticalChainNumAdd);
    l_aChainNumRecord[Tag].ChainNum2 = ChainNum;
    l_aChainNumRecord[Tag].Mcu2Valid = TRUE;
    SGE_WriteChainNumReg(0, FALSE);

    LeaveCriticalSection(&l_CriticalChainNumAdd);
}

void SGE_DiscardChainNumForMCU1(U8 Tag)
{
    EnterCriticalSection(&l_CriticalChainNumAdd);
    l_aChainNumRecord[Tag].Mcu1DiscardChain = TRUE;
    LeaveCriticalSection(&l_CriticalChainNumAdd);
}

void SGE_DiscardChainNumForMCU2(U8 Tag)
{
    EnterCriticalSection(&l_CriticalChainNumAdd);
    l_aChainNumRecord[Tag].Mcu2DiscardChain = TRUE;
    LeaveCriticalSection(&l_CriticalChainNumAdd);
}

/*****************************************************************************
 Prototype      : IsAllChainFinished
 Description    : Check the drq or dwq of the command (tag) have been finis-
                  hed or not.
 Input          : U8 Tag
 Output         : None
 Return Value   : LOCAL
 Calls          :
 Called By      :
 Usage :
          one or both of the two MCUs occurs an error ,the chains(DRQ/DWQ) will be discarded
          and  the process would be force to finish.
          one or both of the two MCUs have not finished yet ,the function return false for continuing.
          if both of the MCU Valid had been set ,then the sum of numbers of two MCU's chains would
          be compared with the finish chain num.True means the command (tag) has completed
          False means continue processing the command
 History        :
 1.Date         : 2013/9/29
   Author       : Gavin Yin
   Modification : Created function

*****************************************************************************/
LOCAL BOOL IsAllChainFinished(U8 Tag)
{
    if(FALSE == l_aChainNumRecord[Tag].Mcu1Valid               //   One or both MCU not completed
        || FALSE == l_aChainNumRecord[Tag].Mcu2Valid)
    {
        return FALSE;
    }
    else if(l_aChainNumRecord[Tag].FinishChainNum                   //   if all chains has been completed
        == l_aChainNumRecord[Tag].ChainNum1 + l_aChainNumRecord[Tag].ChainNum2)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}
/****************************************************************
FUNC: AddFinishiChainNum
INPUT: U8 Tag ---  32 HID (cmd tag)
OUTPUT : void
DESCRIPTION:
    Add local finish cnt after model complete req process
*****************************************************************/
LOCAL void AddFinishChainNum(U8 Tag)
{
    EnterCriticalSection(&l_CriticalChainNumAdd);
    l_aChainNumRecord[Tag].FinishChainNum++;
    LeaveCriticalSection(&l_CriticalChainNumAdd);
    return;
}

/****************************************************************
FUNC: ResetChainNumRecord
INPUT: U8 Tag ---  32 HID (cmd tag)
OUTPUT : void
DESCRIPTION:
    Clear <tag> entry of local chain number record queue
*****************************************************************/
LOCAL void ResetChainNumRecord(U8 Tag)
{
    EnterCriticalSection(&l_CriticalChainNumAdd);
    l_aChainNumRecord[Tag].ulChainNumRec[0]= 0;
    l_aChainNumRecord[Tag].ulChainNumRec[1]= 0;
    LeaveCriticalSection(&l_CriticalChainNumAdd);
    return;
}
/****************************************************************
FUNC: DrqIsFull
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : BOOL   TRUE: FULL   FALSE: NOT FULL
DESCRIPTION:
    Check if the drq of MCU1/MCU2 is full or not.
*****************************************************************/
BOOL DrqIsFull(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,TRUE);
    return tReg.bsFull;
}

/****************************************************************
FUNC: DrqIsEmpty
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : BOOL   TRUE: EMPTY   FALSE: NOT EMPTY
DESCRIPTION:
    Check if the drq of MCU1/MCU2 is EMPTY or not.
*****************************************************************/
BOOL DrqIsEmpty(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,TRUE);
    return tReg.bsEmpty;
}

/****************************************************************
FUNC: DrqReadPtr
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : U8 read ptr of DRQ
DESCRIPTION:
    Get read ptr of DRQ
*****************************************************************/
U8 DrqReadPtr(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,TRUE);
    return tReg.bsRdPtr;
}
/****************************************************************
FUNC: DrqMoveReadPtr
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : void
DESCRIPTION:
    After SGE model process a drq entry successfully ,the read ptr of drq will be accumulated
*****************************************************************/
void DrqMoveReadPtr(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    EnterCriticalSection(&l_CriticalDrq[GrpSel]);

    tReg.ulValue = SGE_ReadStatusReg(GrpSel,TRUE);

    if (l_ulDrqDepth[GrpSel] == ++tReg.bsRdPtr)
    {
        tReg.bsRdPtr = 0;
    }
    tReg.bsEmpty = (tReg.bsRdPtr == tReg.bsWtPtr);
    tReg.bsFull = 0;

    SGE_WriteStatusReg(GrpSel,tReg.ulValue,TRUE);

    LeaveCriticalSection(&l_CriticalDrq[GrpSel]);
}
/****************************************************************
FUNC: SGE_DrqMoveWritePtr
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : void
DESCRIPTION:
    After SGE fw build a drq entry successfully ,the write ptr of drq will be accumulated
*****************************************************************/
void SGE_DrqMoveWritePtr(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    EnterCriticalSection(&l_CriticalDrq[GrpSel]);
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,TRUE);

    if (l_ulDrqDepth[GrpSel] == ++tReg.bsWtPtr)
    {
        tReg.bsWtPtr = 0;
    }
    tReg.bsFull = (tReg.bsWtPtr == tReg.bsRdPtr);
    tReg.bsEmpty = 0;

    SGE_WriteStatusReg(GrpSel,tReg.ulValue,TRUE);
    LeaveCriticalSection(&l_CriticalDrq[GrpSel]);
}
/****************************************************************
FUNC: DwqIsEmpty
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : BOOL   TRUE: EMPTY   FALSE: NOT EMPTY
DESCRIPTION:
    Check if the dwq of MCU1/MCU2 is EMPTY or not.
*****************************************************************/
BOOL DwqIsEmpty(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,FALSE);
    return tReg.bsEmpty;
}
/****************************************************************
FUNC: DwqIsFull
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : BOOL   TRUE: FULL   FALSE: NOT FULL
DESCRIPTION:
    Check if the dwq of MCU1/MCU2 is EMPTY or not.
*****************************************************************/
BOOL DwqIsFull(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,FALSE);
    return tReg.bsFull;
}
/****************************************************************
FUNC: DwqMoveReadPtr
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : void
DESCRIPTION:
    After SGE model process a dwq entry successfully ,the read ptr of dwq will be accumulated
*****************************************************************/
U8 DwqReadPtr(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,FALSE);
    return tReg.bsRdPtr;
}
/****************************************************************
FUNC: SGE_DwqMoveReadPtr
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : void
DESCRIPTION:
    After SGE fw build a dwq entry successfully ,the write ptr of dwq will be accumulated
*****************************************************************/
void DwqMoveReadPtr(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    EnterCriticalSection(&l_CriticalDwq[GrpSel]);
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,FALSE);

    if (l_ulDrqDepth[GrpSel] == ++tReg.bsRdPtr)
    {
        tReg.bsRdPtr = 0;
    }
    tReg.bsEmpty = (tReg.bsRdPtr == tReg.bsWtPtr);
    tReg.bsFull = 0;

    SGE_WriteStatusReg(GrpSel,tReg.ulValue,FALSE);

    LeaveCriticalSection(&l_CriticalDwq[GrpSel]);
}
/****************************************************************
FUNC: SGE_DwqMoveWritePtr
INPUT: U8 GrpSel ---  0: MCU1   ,1: MCU2
OUTPUT : void
DESCRIPTION:
    After SGE fw build a dwq entry successfully ,the write ptr of dwq will be accumulated
*****************************************************************/
void SGE_DwqMoveWritePtr(U8 GrpSel)
{
    DRQ_DWQ_REG tReg;
    EnterCriticalSection(&l_CriticalDwq[GrpSel]);
    tReg.ulValue = SGE_ReadStatusReg(GrpSel,FALSE);

    if (l_ulDrqDepth[GrpSel] == ++tReg.bsWtPtr)
    {
        tReg.bsWtPtr = 0;
    }
    tReg.bsFull = (tReg.bsWtPtr == tReg.bsRdPtr);
    tReg.bsEmpty = 0;

    SGE_WriteStatusReg(GrpSel,tReg.ulValue,FALSE);
    LeaveCriticalSection(&l_CriticalDwq[GrpSel]);
}
/*****************************************************************************
 Prototype      : ProcessDrqEntry
 Description    : DRQ process function

 Input          : SGE_ENTRY *PDrqEntry
 Output         : None
 Return Value   :  void
 Calls          :
 Called By      :
 Usage:
         1. Get HSG ID and DSG ID from input DRQ Entry
         2. Fetch HSG and DSG
         3. Build DRAM requset according to Xferlength of each HSG and DSG
             eg:   total length is 40
                     HSG:  16-> 4 -> 20
                     DSG:  14-> 8 -> 18

                     REQ:  14-> 2 -> 4 -> 2 -> 18
          4. Release HSG and DSG

 History        :
 1.Date         : 2013/9/29
   Author       : Gavin Yin
   Modification : Created function

*****************************************************************************/
LOCAL void ProcessDrqEntry(SGE_ENTRY *PDrqEntry)
{
    U32 ulCurHsgRemLen = 0 ,ulCurDsgRemLen = 0;
    U32 ulXferLen;
    HSG_ENTRY tCurHsg;
    NORMAL_DSG_ENTRY tCurDsg;
    U32 ulHsgId ,ulDsgId;
    LARGE_INTEGER ullHostAddr;

    ulHsgId = PDrqEntry->bsHsgPtr;
    ulDsgId = PDrqEntry->bsDsgPtr;
    do
    {
        if(0 == ulCurHsgRemLen)
        {
            HSG_FetchHsg(ulHsgId ,&tCurHsg);
            ulCurHsgRemLen = tCurHsg.bsLength;
        }

        if(0 == ulCurDsgRemLen)
        {
            DSG_FetchNormalDsg(ulDsgId ,&tCurDsg);
            ulCurDsgRemLen = tCurDsg.bsXferByteLen;
        }

        ulXferLen = (ulCurHsgRemLen > ulCurDsgRemLen) ?
                    ulCurDsgRemLen : ulCurHsgRemLen;

        /* calc host address */
        ullHostAddr.HighPart = tCurHsg.ulHostAddrHigh;
        ullHostAddr.LowPart = tCurHsg.ulHostAddrLow;
        ullHostAddr.QuadPart += (tCurHsg.bsLength - ulCurHsgRemLen);

        Host_ReadFromDram(
                      ullHostAddr.HighPart,
                      ullHostAddr.LowPart,
                      (tCurDsg.bsDramAddr << 1) + (tCurDsg.bsXferByteLen - ulCurDsgRemLen),
                      ulXferLen,
                      PDrqEntry->bsHID);

        ulCurDsgRemLen -= ulXferLen;
        ulCurHsgRemLen -= ulXferLen;
        if(0 == ulCurDsgRemLen)
        {
            DSG_ReleaseNormalDsg(ulDsgId);
            ulDsgId = tCurDsg.bsNextDsgId;
        }

        if(0 == ulCurHsgRemLen)
        {
            HSG_ReleaseHsg(ulHsgId);
            ulHsgId = tCurHsg.bsNextHsgId;
        }

        l_ulCurDrqTotalLen += ulXferLen;
    }while(FALSE == tCurHsg.bsLast || FALSE == tCurDsg.bsLast);
}
/****************************************************************
FUNC: ProcessDwqEntry
INPUT: SGE_ENTRY *PDwqEntry   point to DWQ entry be process
OUTPUT : void
DESCRIPTION:
    SGE model process DWQ entry
*****************************************************************/
LOCAL void ProcessDwqEntry(SGE_ENTRY *PDwqEntry)
{
    U32 ulCurHsgRemLen = 0 ,ulCurDsgRemLen = 0;
    U32 ulXferLen;
    HSG_ENTRY tCurHsg;
    NORMAL_DSG_ENTRY tCurDsg;
    U32 ulHsgId ,ulDsgId;
    LARGE_INTEGER ullHostAddr;

    ulHsgId = PDwqEntry->bsHsgPtr;
    ulDsgId = PDwqEntry->bsDsgPtr;
    do
    {
        if(0 == ulCurHsgRemLen)
        {
            HSG_FetchHsg(ulHsgId ,&tCurHsg);
            ulCurHsgRemLen = tCurHsg.bsLength;
        }

        if(0 == ulCurDsgRemLen)
        {
            DSG_FetchNormalDsg(ulDsgId ,&tCurDsg);
            ulCurDsgRemLen = tCurDsg.bsXferByteLen;
        }

        ulXferLen = (ulCurHsgRemLen > ulCurDsgRemLen) ?
                    ulCurDsgRemLen : ulCurHsgRemLen;

        /* calc host address */
        ullHostAddr.HighPart = tCurHsg.ulHostAddrHigh;
        ullHostAddr.LowPart = tCurHsg.ulHostAddrLow;
        ullHostAddr.QuadPart += (tCurHsg.bsLength - ulCurHsgRemLen);
        Host_WriteToDram(
                      ullHostAddr.HighPart ,
                      ullHostAddr.LowPart,
                      (tCurDsg.bsDramAddr << 1) + (tCurDsg.bsXferByteLen - ulCurDsgRemLen),
                      ulXferLen,
                      PDwqEntry->bsHID);
        ulCurDsgRemLen -= ulXferLen;
        ulCurHsgRemLen -= ulXferLen;
        if(0 == ulCurDsgRemLen)
        {
            DSG_ReleaseNormalDsg(ulDsgId);
            ulDsgId = tCurDsg.bsNextDsgId;
        }

        if(0 == ulCurHsgRemLen)
        {
            HSG_ReleaseHsg(ulHsgId);
            ulHsgId = tCurHsg.bsNextHsgId;
        }

        l_ulCurDwqTotalLen += ulXferLen;
    }while(FALSE == tCurHsg.bsLast || FALSE == tCurDsg.bsLast);
}

#if defined(SIM)

BOOL SGE_ModelThreadExit()
{
    l_bSGEThreadExit = TRUE;
    SGE_ModelSchedule();
    WaitForSingleObject(g_hSgeThread,INFINITE);
    CloseHandle(g_hSgeThread);

    return TRUE;
}

extern U32 SIM_DevGetStatus();

DWORD WINAPI SGE_ModelThread(LPVOID p)
{


    l_bSGEThreadExit = FALSE;

    while (FALSE == l_bSGEThreadExit)
    {

        WaitForSingleObject(g_SimSgeEvent,INFINITE);
        SGE_ModelProcess();
    }
    return 0;
}

void SGE_ModelSchedule(void)
{
#ifdef NO_THREAD
    SGE_ModelProcess();
#else
    SetEvent(g_SimSgeEvent);
    //ReleaseSemaphore( g_SimSgeEvent ,1 ,NULL );
#endif
}

#elif defined(SIM_XTENSA)

void SGE_ModelThread_XTMP(void)
{
    while(1){
        SGE_ModelProcess();
        XTMP_wait(1);
    }

    return;
}
#endif   ///   SIM / SIM_XTENSA


//  critical section var init
void SGE_LockInit(void)
{
    U8 ucGrpSel;
    for (ucGrpSel = 0;ucGrpSel < GROUP_NUM;ucGrpSel++)
    {
        InitializeCriticalSection(&l_CriticalDrq[ucGrpSel]);
        InitializeCriticalSection(&l_CriticalDwq[ucGrpSel]);
    }

    InitializeCriticalSection(&l_CriticalChainNumAdd);
    InitializeCriticalSection(&l_CriticalChainNumCheck);
    InitializeCriticalSection(&g_CriticalChainNumFw);
}

#ifndef IGNORE_PERFORMANCE
void SGE_PerformaceRcordInit(void)
{
    memset((void *)&l_tSgeDrqTimeRecord, 0, sizeof(ST_TIME_RECORD));
    memset((void *)&l_tSgeDwqTimeRecord, 0, sizeof(ST_TIME_RECORD));
    memset((void *)&l_ucSgeDrqPcieSts ,SGE_STS_WAIT_CMD, DRQ_DWQ_DROUP);
    memset((void *)&l_ucSgeDwqPcieSts ,SGE_STS_WAIT_CMD, DRQ_DWQ_DROUP);
    return;
}
#endif

void SGE_ParamInit(void)
{
    U32 ulIndex = 0;
    U32 *pAddr = 0;
    for (ulIndex = 0; ulIndex < HID_TOTAL; ulIndex++)
    {
        pAddr = (U32*)&l_aChainNumRecord[ulIndex];
        *(pAddr) = 0;
        *(pAddr+1) = 0;
    }

    rDrqMcu1Status = 0x4;
    rDwqMcu1Status = 0x4;
    rDrqMcu2Status = 0x4; // | (DRQ_GROUP1_BEGIN << 16)|(DRQ_GROUP1_BEGIN << 24);
    rDwqMcu2Status = 0x4; // | (DWQ_GROUP1_BEGIN << 16)|(DWQ_GROUP1_BEGIN << 24);

    rSgqStatusLo = 0;
    rSgqStatusHi = 0;


    rDwqMcu0Status = 0x4;
    rDrqMcu0Status = 0x4;



}


// model init  (SIM / SIM_XTENSA)
void SGE_ModelInit(void)
{
#if defined(SIM_XTENSA)
    U32 ulBuf;

    SGE_LockInit();

    ulBuf=0x4;
    Comm_WriteReg((U32)&rDrqMcu1Status,1,&ulBuf);
    Comm_WriteReg((U32)&rDrqMcu2Status,1,&ulBuf);
    Comm_WriteReg((U32)&rDwqMcu1Status,1,&ulBuf);
    Comm_WriteReg((U32)&rDwqMcu2Status,1,&ulBuf);

    Comm_WriteReg((U32)&rDwqMcu0Status,1,&ulBuf);
    Comm_WriteReg((U32)&rDrqMcu0Status,1,&ulBuf);


    Comm_HsgModelInit();
    Comm_NormalDsgModelInit();

#ifndef IGNORE_PERFORMANCE
    SGE_PerformaceRcordInit();
#endif
    XTMP_userThreadNew("SGE_ModelThread_XTMP" ,(XTMP_userThreadFunction)SGE_ModelThread_XTMP ,NULL);
#elif defined(SIM)

    Comm_HsgModelInit();
    Comm_NormalDsgModelInit();
    #ifndef NO_THREAD
        g_SimSgeEvent = CreateEvent(NULL ,FALSE ,FALSE ,NULL);
        //g_SimSgeEvent = CreateSemaphore( NULL ,0 ,512 ,NULL );
        g_hSgeThread = CreateThread(0 ,0 ,SGE_ModelThread ,0 ,0 ,0);
    #endif

    SGE_LockInit();

    SGE_ParamInit();

#endif
    SGE_LocalParamInit();
}
/*****************************************************************************
 Prototype      : SGE_GetDrqEntry
 Description    : SGE model get a DRQ entry to process by offset
*****************************************************************************/
void SGE_GetDrqEntry(U8 ucGrpSel,SGE_ENTRY* pDrqEntry)
{
    U8 ucReadPtr = DrqReadPtr(ucGrpSel);
#ifdef SIM
    *pDrqEntry = *(SGE_ENTRY*)(l_aDrqBaseAddr[ucGrpSel] + (ucReadPtr * sizeof(SGE_ENTRY)));
#else
    Comm_ReadReg((U32)(l_aDrqBaseAddr[ucGrpSel] + (ucReadPtr * sizeof(SGE_ENTRY))),sizeof(SGE_ENTRY)/sizeof(U32),(U32*)pDrqEntry);
#endif
}
/*****************************************************************************
 Prototype      : SGE_GetDwqEntry
 Description    : SGE model get a DWQ entry to process by offset
*****************************************************************************/
void SGE_GetDwqEntry(U8 ucGrpSel,SGE_ENTRY* pDwqEntry)
{
    U8 ucReadPtr = DwqReadPtr(ucGrpSel);
#ifdef SIM
    *pDwqEntry = *(SGE_ENTRY*)(l_aDwqBaseAddr[ucGrpSel] + (ucReadPtr * sizeof(SGE_ENTRY)));
#else
    Comm_ReadReg((U32)(l_aDwqBaseAddr[ucGrpSel] + (ucReadPtr * sizeof(SGE_ENTRY))),sizeof(SGE_ENTRY)/sizeof(U32),(U32*)pDwqEntry);
#endif
}

/*****************************************************************************
 Prototype      : SGE_CheckChainNumComplete
 Description    : check if the cmd is complete or not by checking the total chain number
 Input          : tag (hid)
 Output         : None
 Return Value   :
 Calls          :
 Called By      :   HAL_SgeFinishChainCnt , SGE_ModelProcess

 History        :
 1.Date         : 2014/6/24
   Author       :  Victor Zhang
   Modification : Created function
 2.Data         : 2014/7/3
   Author       : Victor
   Modification : Add finish flag check before reset chain number record

Note:
    Called when the chain number register be set ,and data transfer end

*****************************************************************************/
void SGE_CheckChainNumComplete(U8 ucTag)
{
    EnterCriticalSection(&l_CriticalChainNumCheck);
    if(TRUE == IsAllChainFinished(ucTag))
    {
#if 0
        //in some case, there is no data transfering on SGE, we should not DBG_Break for this case
        if (0 == g_uCMDFinishFlag[ucTag] && 0 != l_aChainNumRecord[ucTag].FinishChainNum)
        {
            DBG_Break();
        }
#endif
        ResetChainNumRecord(ucTag);
        SIM_HostCWBQDataDoneTrigger( ucTag );
    }
    LeaveCriticalSection(&l_CriticalChainNumCheck);
}

/*****************************************************************************
 Prototype      : SGE_ModelProcess
 Description    : model process
 Input          : void
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2013/9/29
   Author       :  Gavin Yin
   Modification : Created function

*****************************************************************************/
void SGE_ModelProcess(void)
{
    U8 ucGrpSel;
#ifdef IGNORE_PERFORMANCE
    SGE_ENTRY tDrqEntry;
    SGE_ENTRY tDwqEntry;
#else
    U32 ulCurSecCnt;
    U32 aCurDrqTime;
    U32 aCurDwqTime;
    static U32 ulDrqTotalLen[DRQ_DWQ_DROUP] = {0x0, 0x0};
    static U32 ulDwqTotalLen[DRQ_DWQ_DROUP] = {0x0, 0x0};
    static U8 ucDrqHID[DRQ_DWQ_DROUP] = {0xff, 0xff};
    static U8 ucDwqHID[DRQ_DWQ_DROUP] = {0xff, 0xff};
#endif
    for(ucGrpSel = 0; ucGrpSel < GROUP_NUM; ucGrpSel++)
    {
        /* check DRQ */
        if(FALSE == DrqIsEmpty(ucGrpSel))
        {
#ifdef IGNORE_PERFORMANCE
            SGE_GetDrqEntry(ucGrpSel,&tDrqEntry);
            ProcessDrqEntry(&tDrqEntry);
            DrqMoveReadPtr(ucGrpSel);

            if (0 != ucGrpSel)    // MCU0 does not update chain number
            {
                AddFinishChainNum(tDrqEntry.bsHID);
                SGE_CheckChainNumComplete(tDrqEntry.bsHID);
            }
#else
            switch(l_ucSgeDrqPcieSts[ucGrpSel])
            {
            case SGE_STS_WAIT_CMD:
                {
                    SGE_ENTRY tDrqEntry;
                    SGE_GetDrqEntry(ucGrpSel,&tDrqEntry);
                    ProcessDrqEntry(&tDrqEntry);
                    ucDrqHID[ucGrpSel] = tDrqEntry.bsHID;
                    ulDrqTotalLen[ucGrpSel] = l_ulCurDrqTotalLen;

                    l_ulCurDrqTotalLen = 0;
                    l_ucSgeDrqPcieSts[ucGrpSel] = SGE_STS_WAIT_BUS;
                }
                break;

            case SGE_STS_WAIT_BUS:
                {
                    EnterCriticalSection(&g_PcieDevToHostCriticalSection);
                    if(PCIE_STS_FREE == l_ucPcieBusDevToHostSts)
                    {
                        l_ucPcieBusDevToHostSts =  PCIE_STS_BUSY;
                        l_tSgeDrqTimeRecord[ucGrpSel].time_start = (U32)GET_TIME();
                        ulCurSecCnt = ulDrqTotalLen[ucGrpSel]/SEC_SIZE;
                        l_tSgeDrqTimeRecord[ucGrpSel].time_busy = ((ulCurSecCnt * BUSY_TIME_DEV_TO_HOST_PER_BUF) / SEC_PER_BUF);

                        l_ucSgeDrqPcieSts[ucGrpSel] = SGE_STS_PCIE_TRANS;
                    }
                    LeaveCriticalSection(&g_PcieDevToHostCriticalSection);
                }
                break;

            case SGE_STS_PCIE_TRANS:
                {
                    aCurDrqTime = (U32)GET_TIME();
                    if((aCurDrqTime - l_tSgeDrqTimeRecord[ucGrpSel].time_start) >= l_tSgeDrqTimeRecord[ucGrpSel].time_busy)
                    {
                        DrqMoveReadPtr(ucGrpSel);
                        if (0 != ucGrpSel)    // MCU0 does not update chain number
                        {
                            AddFinishChainNum(ucDrqHID[ucGrpSel]);
                            SGE_CheckChainNumComplete(ucDrqHID[ucGrpSel]);
                        }
                        EnterCriticalSection(&g_PcieDevToHostCriticalSection);
                        l_ucPcieBusDevToHostSts = PCIE_STS_FREE;
                        LeaveCriticalSection(&g_PcieDevToHostCriticalSection);
                        l_ucSgeDrqPcieSts[ucGrpSel] = SGE_STS_WAIT_CMD;
                    }
                }
                break;

            default:
                break;
            }
#endif
        }

        /* check DWQ */
        if(FALSE == DwqIsEmpty(ucGrpSel))
        {
#ifdef IGNORE_PERFORMANCE
            SGE_GetDwqEntry(ucGrpSel,&tDwqEntry);
            ProcessDwqEntry(&tDwqEntry);
            DwqMoveReadPtr(ucGrpSel);
            if (0 != ucGrpSel)    // MCU0 does not update chain number
            {
                AddFinishChainNum(tDwqEntry.bsHID);
                SGE_CheckChainNumComplete(tDwqEntry.bsHID);
            }
#else
            switch(l_ucSgeDwqPcieSts[ucGrpSel])
            {
            case SGE_STS_WAIT_CMD:
                {
                    SGE_ENTRY tDwqEntry;
                    SGE_GetDwqEntry(ucGrpSel,&tDwqEntry);
                    ProcessDwqEntry(&tDwqEntry);
                    ucDwqHID[ucGrpSel] = tDwqEntry.bsHID;
                    ulDwqTotalLen[ucGrpSel] = l_ulCurDwqTotalLen;

                    l_ulCurDwqTotalLen = 0;
                    l_ucSgeDwqPcieSts[ucGrpSel] = SGE_STS_WAIT_BUS;
                }
                break;

            case SGE_STS_WAIT_BUS:
                {
                    EnterCriticalSection(&g_PcieHostToDevCriticalSection);
                    if(PCIE_STS_FREE == l_ucPcieBusHostToDevSts)
                    {
                        l_ucPcieBusHostToDevSts = PCIE_STS_BUSY;
                        l_tSgeDwqTimeRecord[ucGrpSel].time_start = (U32)GET_TIME();
                        ulCurSecCnt = ulDwqTotalLen[ucGrpSel]/SEC_SIZE;
                        l_tSgeDwqTimeRecord[ucGrpSel].time_busy = ((ulCurSecCnt * BUSY_TIME_HOST_TO_DEV_PER_BUF) / SEC_PER_BUF);

                        l_ucSgeDwqPcieSts[ucGrpSel] = SGE_STS_PCIE_TRANS;
                    }
                    LeaveCriticalSection(&g_PcieHostToDevCriticalSection);
                }
                break;

            case SGE_STS_PCIE_TRANS:
                {
                    aCurDwqTime = (U32)GET_TIME();
                    if((aCurDwqTime - l_tSgeDwqTimeRecord[ucGrpSel].time_start) >= l_tSgeDwqTimeRecord[ucGrpSel].time_busy)
                    {
                        DwqMoveReadPtr(ucGrpSel);
                        
                        if (0 != ucGrpSel)    // MCU0 does not update chain number
                        {
                            AddFinishChainNum(ucDwqHID[ucGrpSel]);
                            SGE_CheckChainNumComplete(ucDwqHID[ucGrpSel]);
                        }
                        l_ucSgeDwqPcieSts[ucGrpSel] = SGE_STS_WAIT_CMD;
                        EnterCriticalSection(&g_PcieHostToDevCriticalSection);
                        l_ucPcieBusHostToDevSts = PCIE_STS_FREE;
                        LeaveCriticalSection(&g_PcieHostToDevCriticalSection);
                    }
                }
                break;

            default:
                break;

            }
#endif
        }
    }
    return;
}
/*------------------------------------------------------------------------------
Name: SGE_OtfbToHost
Description:
    In on-the-fly read case ,after NFC model push data to OTFB ,NFC model call this fucntion to
    trigger SGE transfer OTFB data to host.
Input Param:
    U8 PU: pu number
    U8 Level: level of pu hardware queue
Output Param:
    none
Return Value:
    void
Usage:
    in windows simulation ,nfc model call this function to trigger SGE transfer OTFB data to host
History:
    20130910    Gavin   created
    20140815    Jason   nfc err, no need add chainnumber
------------------------------------------------------------------------------*/
void SGE_OtfbToHost(U8 ucPhyPu ,U8 ucLun, U8 ucLevel, U32 bNfcInjErr)
{
    HSG_ENTRY tHsgEntry;
    U16 usNextHsgId;
    SGE_ENTRY *pSgqEntry;
    U32 ulOtfbAddr;
    U32 ulLastHsgLen = 0;

    ulOtfbAddr = SGE_GetOtfbAddr(ucPhyPu);

    pSgqEntry = (SGE_ENTRY*)SGE_GetSgqEntry(ucPhyPu, ucLun, ucLevel);

    usNextHsgId = pSgqEntry->bsHsgPtr;

    do
    {
        HSG_FetchHsg(usNextHsgId ,&tHsgEntry);
        if ((NF_SUCCESS != bNfcInjErr) && (NF_ERR_TYPE_RECC != bNfcInjErr))
        {
            tHsgEntry.bsLength = 0;
        }

#if (defined(HAL_UNIT_TEST) || defined(L3_UNIT_TEST))
        //XXX_OtfbToDram (tHsgEntry.HostAddrHigh ,tHsgEntry.HostAddrLow ,ulOtfbAddr+ulLastHsgLen ,tHsgEntry.Length);
#else
        Host_ReadFromOTFB(tHsgEntry.ulHostAddrHigh,
                            tHsgEntry.ulHostAddrLow,
                            ulOtfbAddr+ulLastHsgLen,
                            tHsgEntry.bsLength,
                            pSgqEntry->bsHID);
#endif
        ulLastHsgLen += tHsgEntry.bsLength;

        //Realse HSG
        HSG_ReleaseHsg(usNextHsgId);
        usNextHsgId = tHsgEntry.bsNextHsgId;
    }while (TRUE != tHsgEntry.bsLast);

    if ((NF_SUCCESS == bNfcInjErr) || (NF_ERR_TYPE_RECC == bNfcInjErr))
    {
        AddFinishChainNum(pSgqEntry->bsHID);
        SGE_CheckChainNumComplete(pSgqEntry->bsHID);
    }

    return;
}

U32 SGE_GetSgqEntry(U8 ucPhyPu ,U8 ucLun, U8 ucLevel)
{
    U32 ulSgeEntryId = ucPhyPu* NFCQ_DEPTH_TOTAL + ucLun *NFCQ_DEPTH + ucLevel;
#ifdef SIM
    U32 ulSgeEntry = SGQ_BASE_ADDR + (ulSgeEntryId*sizeof(SGE_ENTRY));
#else
    U32 ulSgeEntry = (U32)GetVirtualAddrInLocalMem(SGQ_BASE_ADDR + (ulSgeEntryId*sizeof(SGE_ENTRY));//SGQ_BASE_ADDR + (ulSgeEntryId*sizeof(SGE_ENTRY) - SRAM0_START_ADDRESS + (U32)g_pLocalRam0;
#endif

    return ulSgeEntry;
}

/*------------------------------------------------------------------------------
Name: HSG_InitHsg
Description:
    initialize HSG base address; set HSG pool to default status.
Input Param:
    void
Output Param:
    none
Return Value:
    void
Usage:
    in model init stage ,call this function to initialize HSG;
    any other HSG functions can be called after this function finished.
History:
    20130828    Gavin   created
------------------------------------------------------------------------------*/
void SGE_HostToOtfb(U8 ucPhyPU ,U8 ucLun, U8 ucLevel)
{
    HSG_ENTRY tHsgEntry;
    U16 usNextHsgId;
    SGE_ENTRY *pSgqEntry;
    U32 ulOtfbAddr;
    U32 ulLastHsgLen = 0;

    ulOtfbAddr = SGE_GetOtfbAddr(ucPhyPU);

    pSgqEntry = (SGE_ENTRY*)SGE_GetSgqEntry(ucPhyPU, ucLun, ucLevel);

    usNextHsgId = pSgqEntry->bsHsgPtr;

    do
    {
        HSG_FetchHsg(usNextHsgId ,&tHsgEntry);
#if (defined(HAL_UNIT_TEST) || defined(L3_UNIT_TEST))
        //XXX_DramToOtfb (tHsgEntry.HostAddrHigh ,tHsgEntry.HostAddrLow ,ulOtfbAddr + ulLastHsgLen ,tHsgEntry.Length);
#else
        Host_WriteToOTFB(tHsgEntry.ulHostAddrHigh,
                            tHsgEntry.ulHostAddrLow,
                            ulOtfbAddr+ulLastHsgLen,
                            tHsgEntry.bsLength,
                            pSgqEntry->bsHID);
#endif
        ulLastHsgLen += tHsgEntry.bsLength;

        //Realse HSG
        HSG_ReleaseHsg(usNextHsgId);
        usNextHsgId = tHsgEntry.bsNextHsgId;
    }while (TRUE != tHsgEntry.bsLast);

    AddFinishChainNum(pSgqEntry->bsHID);
    SGE_CheckChainNumComplete(pSgqEntry->bsHID);

    return;
}

/*------------------------------------------------------------------------------
Name: SGE_GetOtfbAddr
Description:
    In on-the-fly case ,NFC model call this function to get OTFB address for read/write data
Input Param:
    U8 PU: pu number
Output Param:
    none
Return Value:
    U32: address offset in OTFB
Usage:
    in windows simulation ,nfc model call this function to get OTFB address for PU read/write
History:
    20130910    Gavin   created
    20170313    Jason   Otfb-Buf is only a tmp buf in data-tx in our winsim model design for simple.
                        so, we can default using the Ch#0's buf for avoding overflow when host-read size > 32KB.
                        next: we should to make it as a real data-buf for improving winsim performance later.
------------------------------------------------------------------------------*/
U32 SGE_GetOtfbAddr(U8 ucPhyPU)
{
#ifdef XOR_ENABLE
    U8 ucChNum = 0;
#else
    U8 ucChNum = ucPhyPU % NFC_CH_TOTAL;
#endif

    return (OTFB_SGE_FLY_BASE + ucChNum * ON_THE_FLY_PER_CH_SIZE - OTFB_START_ADDRESS);
}


/***********************************************************
FUNC : DwqDrqRegWrite
INPUT :
    regaddr --- SGE register offset
    regvalue --- SGE register value
OUTPUT :
    TRUE ---  write through
    FALSE --- write back

DESCRIPTION:
    SGE register update call back function under XTMP mode

**********************************************************/

BOOL DwqDrqRegWrite(U32 regaddr ,U32 regvalue ,U32 nsize)
{
    DRQ_DWQ_REG tDrqMcu0Status;
    DRQ_DWQ_REG tDwqMcu0Status;
    DRQ_DWQ_REG tDrqMcu1Status;
    DRQ_DWQ_REG tDrqMcu2Status;
    DRQ_DWQ_REG tDwqMcu1Status;
    DRQ_DWQ_REG tDwqMcu2Status;
    CHAIN_NUM_REG tChainNumMcu1;
    CHAIN_NUM_REG tChainNumMcu2;

    // DRQ status for MCU1 set Registers
    if(regaddr >= (U32)(&rDrqMcu0Status) && regaddr < ((U32)(&rDrqMcu0Status)+sizeof(U32)))
    {
        tDrqMcu0Status.ulValue = regvalue;
        if(TRUE == tDrqMcu0Status.bsTrig)
        {
            tDrqMcu0Status.bsTrig = FALSE;
            SGE_DrqMoveWritePtr(0);
        }
    }
    if(regaddr >= (U32)(&rDrqMcu1Status) && regaddr < ((U32)(&rDrqMcu1Status)+sizeof(U32)))
    {
        tDrqMcu1Status.ulValue = regvalue;
        if(TRUE == tDrqMcu1Status.bsTrig)
        {
            tDrqMcu1Status.bsTrig = FALSE;
            SGE_DrqMoveWritePtr(1);
        }
    }
    if(regaddr >= (U32)(&rDrqMcu2Status) && regaddr < ((U32)(&rDrqMcu2Status)+sizeof(U32)))
    {
        tDrqMcu2Status.ulValue = regvalue;
        if(TRUE == tDrqMcu2Status.bsTrig)
        {
            tDrqMcu2Status.bsTrig = FALSE;
            SGE_DrqMoveWritePtr(2);
        }
    }

    if(regaddr >= (U32)(&rDwqMcu0Status) && regaddr < ((U32)(&rDwqMcu0Status)+sizeof(U32)))
    {
        tDwqMcu0Status.ulValue = regvalue;
        if(TRUE == tDwqMcu0Status.bsTrig)
        {
            tDwqMcu0Status.bsTrig = FALSE;
            SGE_DwqMoveWritePtr(0);
        }
    }

    if(regaddr >= (U32)(&rDwqMcu1Status) && regaddr < ((U32)(&rDwqMcu1Status)+sizeof(U32)))
    {
        tDwqMcu1Status.ulValue = regvalue;
        if(TRUE == tDwqMcu1Status.bsTrig)
        {
            tDwqMcu1Status.bsTrig = FALSE;
            SGE_DwqMoveWritePtr(1);
        }
    }

    if(regaddr >= (U32)(&rDwqMcu2Status) && regaddr < ((U32)(&rDwqMcu2Status)+sizeof(U32)))
    {
        tDwqMcu2Status.ulValue = regvalue;
        if(TRUE == tDwqMcu2Status.bsTrig)
        {
            tDwqMcu2Status.bsTrig = FALSE;
            SGE_DwqMoveWritePtr(2);
        }
    }

    if((U32)(&rChainNumMcu1) ==regaddr )
    {
        tChainNumMcu1.ulChainNumReg = regvalue;
        SGE_RecordChainNumForMCU1(tChainNumMcu1.bsHID,tChainNumMcu1.bsChainNum,FALSE);
        SGE_CheckChainNumComplete(tChainNumMcu1.bsHID);
    }
    if((U32)(&rChainNumMcu2) ==regaddr )
    {
        tChainNumMcu2.ulChainNumReg = regvalue;
        SGE_RecordChainNumForMCU2(tChainNumMcu2.bsHID,tChainNumMcu2.bsChainNum,FALSE);
        SGE_CheckChainNumComplete(tChainNumMcu2.bsHID);
    }
    if ((U32)(&rNfcHIDTag1) == regaddr)
    {
        tChainNumMcu1.ulChainNumReg = regvalue;
        SGE_RecordChainNumForMCU1(tChainNumMcu1.bsHID,tChainNumMcu1.bsChainNum,TRUE);
        SGE_CheckChainNumComplete(tChainNumMcu1.bsHID);
    }
    if ((U32)(&rNfcHIDTag2) == regaddr)
    {
        tChainNumMcu2.ulChainNumReg = regvalue;
        SGE_RecordChainNumForMCU2(tChainNumMcu2.bsHID,tChainNumMcu2.bsChainNum,TRUE);
        SGE_CheckChainNumComplete(tChainNumMcu2.bsHID);
    }
    return FALSE;
}

/* end of file sim_SGE.c */

