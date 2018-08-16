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
------------------------------------------------------------------------------*/
#include "sim_HSG.h"
#include "HAL_Xtensa.h"
#include <string.h>
#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#endif

#include "HAL_HSG.h"

extern void Comm_WriteReg(U32 addr, U32 nWords, U32 *buf);
extern void Comm_ReadReg(U32 addr, U32 nWords, U32 *buf);
extern void Comm_WriteOtfbByByte(U32 addr, U32 nBytes, U8 *buf);
extern void Comm_WriteDramByByte(U32 addr, U32 nBytes, U8 *buf);

//GLOBAL HSG_ENTRY * g_p_Hsg_Entry;

LOCAL U16 l_usHsgHead;
LOCAL U16 l_usHsgCount;
LOCAL U16 l_aFreeHsg[HSG_NUM];
LOCAL BOOL l_HsgValidSts[HSG_NUM];
LOCAL CRITICAL_SECTION l_CriticalHsgIndex;
LOCAL HSG_ENTRY * l_pHsgEntryBase;

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
    in model init stage, call this function to initialize HSG;
    any other HSG functions can be called after this function finished.
History:
    20130828    Gavin   created
------------------------------------------------------------------------------*/
void HSG_InitHsg(void)
{
    U16 usHsgIndex;
    U32 ulHsgBaseAddr;
    HSG_REPORT_MCU tHsgReportMcu;
    InitializeCriticalSection(&l_CriticalHsgIndex);

    for (usHsgIndex = 0; usHsgIndex < HSG_NUM; usHsgIndex++)
    {
        l_aFreeHsg[usHsgIndex] = usHsgIndex;
        l_HsgValidSts[usHsgIndex] = FALSE;
    }
    //Initilaze pool
    EnterCriticalSection(&l_CriticalHsgIndex);
    l_usHsgHead = 3;
    l_usHsgCount = HSG_NUM-3;
    LeaveCriticalSection(&l_CriticalHsgIndex);

    //Initilaze registers
    *((U32 *)(&tHsgReportMcu)) = 0;
    tHsgReportMcu.bsHsgId = 2;
    tHsgReportMcu.bsHsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rHsgReportMcu0), 1,(U32 *)(&tHsgReportMcu));

    *((U32 *)(&tHsgReportMcu)) = 0;
    tHsgReportMcu.bsHsgId = 0;
    tHsgReportMcu.bsHsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rHsgReportMcu1), 1,(U32 *)(&tHsgReportMcu));

    *((U32 *)(&tHsgReportMcu)) = 0;
    tHsgReportMcu.bsHsgId = 1;
    tHsgReportMcu.bsHsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rHsgReportMcu2), 1,(U32 *)(&tHsgReportMcu));

    ulHsgBaseAddr = (U32)SATA_DSG_BASE;
    Comm_WriteReg((U32)(&rHsgBaseAddr), 1,&ulHsgBaseAddr);

    return;
}

/*------------------------------------------------------------------------------
Name: HSG_AllocateHsg
Description:
    allocate one HSG from pool.
Input Param:
    U8 McuId: MCU number 1 or 2
Output Param:
    none
Return Value:
    BOOL:
        0 = no HSG got;
        1 = Get one HSG;
Usage:
    in XTMP simulatin, model call this function to get one HSG.
    in Win simulation, this function used as stub fucntion in FW
History:
    20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL HSG_AllocateHsg(U8 McuId)
{
    U8 ucGetHsgCnt = 0;
    U16 CurHsgId;
    HSG_REPORT_MCU tHsgReport;

    if(MCU0_ID == McuId)
    {
        Comm_ReadReg((U32)(&rHsgReportMcu0), 1, (U32 *)(&tHsgReport));
    }

    if(MCU1_ID == McuId)
    {
        Comm_ReadReg((U32)(&rHsgReportMcu1), 1, (U32 *)(&tHsgReport));
    }
    if(MCU2_ID == McuId)
    {
        Comm_ReadReg((U32)(&rHsgReportMcu2), 1, (U32 *)(&tHsgReport));
    }

    EnterCriticalSection(&l_CriticalHsgIndex);
    if (0 == l_usHsgCount)
    {
        CurHsgId = INVALID_4F;
        ucGetHsgCnt = 0;
        tHsgReport.bsHsgValidEn = FALSE;
    }
    else
    {
        CurHsgId = l_aFreeHsg[l_usHsgHead];
        l_usHsgCount--;
        l_usHsgHead++;
        l_usHsgHead %= HSG_NUM;

        ucGetHsgCnt = 1;
        tHsgReport.bsHsgValidEn = TRUE;
    }
    LeaveCriticalSection(&l_CriticalHsgIndex);

    tHsgReport.bsHsgTrigger = FALSE;
    tHsgReport.bsHsgId = CurHsgId;

    if(MCU0_ID == McuId)
    {
        Comm_WriteReg((U32)(&rHsgReportMcu0), 1, (U32 *)(&tHsgReport));
    }

    if(MCU1_ID == McuId)
    {
        Comm_WriteReg((U32)(&rHsgReportMcu1), 1, (U32 *)(&tHsgReport));
    }
    if(MCU2_ID == McuId)
    {
        Comm_WriteReg((U32)(&rHsgReportMcu2), 1, (U32 *)(&tHsgReport));
    }

    return ucGetHsgCnt;
}

/*------------------------------------------------------------------------------
Name: HSG_SetHsgValid
Description:
set one HSG to valid status.
Input Param:
U16 HsgId: HSG id
Output Param:
none
Return Value:
void
Usage:
in XTMP simulatin, this function is used to set one sata DSG to valid.
in Win simulation, this function used as stub fucntion in FW
History:
20130912    Tobey   created
------------------------------------------------------------------------------*/
void HSG_SetHsgValid(U16 HsgId)
{
    if (TRUE == l_HsgValidSts[HsgId])
    {
        DBG_Getch();
    }

    EnterCriticalSection(&l_CriticalHsgIndex);
    l_HsgValidSts[HsgId] = TRUE;
    LeaveCriticalSection(&l_CriticalHsgIndex);

    return;
}

/*------------------------------------------------------------------------------
Name: HSG_SetHsgInvalid
Description:
set one HSG to invalid status.
in winsim add released hsg into pool at the same time.
Input Param:
U16 HsgId: Hsg id
Output Param:
none
Return Value:
void
Usage:
in XTMP simulatin, model call this function to set one HSG to invalid.
in Win simulation, this function used as stub fucntion in FW
History:
20130912    Tobey   created
------------------------------------------------------------------------------*/
void HSG_SetHsgInvalid(U16 HsgId)
{
    if(FALSE == l_HsgValidSts[HsgId])
    {
        DBG_Getch();
    }

    EnterCriticalSection(&l_CriticalHsgIndex);
    l_HsgValidSts[HsgId] = FALSE;
    l_aFreeHsg[(l_usHsgHead + l_usHsgCount)%HSG_NUM] = HsgId;
    l_usHsgCount++;
    LeaveCriticalSection(&l_CriticalHsgIndex);
    
    return;
}

/*------------------------------------------------------------------------------
Name: HSG_IsHsgValid
Description:
check HSG is valid or not.
Input Param:
U16 HsgId: HSG id
Output Param:
none
Return Value:
BOOL: TRUE = DSG is valid; FALSE = DSG is not valid
Usage:
in XTMP/Win simulatin, call this function to check sata DSG is valid or not.
History:
20130912    Tobey   created
------------------------------------------------------------------------------*/
BOOL HSG_IsHsgValid(U16 HsgId)
{
    return l_HsgValidSts[HsgId];
}

/*------------------------------------------------------------------------------
Name: HSG_ReleaseHsg
Description:
    release one HSG to pool after data in this HSG transfered.
Input Param:
    U16 HsgId: HSG id
Output Param:
    none
Return Value:
    void
Usage:
    in XTMP/Win simulatin, model call this function to release HSG.
    in Win simulation, when FW do error handling, this function used as stub fucntion in FW
History:
    20130828    Gavin   created
------------------------------------------------------------------------------*/
void HSG_ReleaseHsg(U16 HsgId)
{
    HSG_SetHsgInvalid(HsgId);
    return;
}

/*------------------------------------------------------------------------------
Name: HSG_FetchHsg
Description:
    get contents of HSG.
Input Param:
    U16 HsgId: HSG id
Output Param:
    HSG_ENTRY *PHsg: pointer to HSG entry
Return Value:
    void
Usage:
    in XTMP/Win simulatin, model call this function to get contents of HSG
History:
    20130828    Gavin   created
------------------------------------------------------------------------------*/
void HSG_FetchHsg(U16 HsgId, HSG_ENTRY *PHsg)
{
    HSG_ENTRY *pHsgTemp;
    pHsgTemp = &l_pHsgEntryBase[HsgId];
    memcpy((void *)PHsg, (void *)pHsgTemp, sizeof(HSG_ENTRY));
}

void Comm_HsgModelInit(void)
{
    HSG_InitHsg();
#ifdef SIM
    l_pHsgEntryBase = (HSG_ENTRY *)SATA_DSG_BASE;
#else
    l_pHsgEntryBase = (HSG_ENTRY *)GetVirtualAddrInLocalMem(SATA_DSG_BASE);
#endif
}
/*------------------------------------------------------------------------------
Name: DSG_UpdateHsgReg
Description:
update Hsg status and pool
Input Param:
pHSG_REPORT_MCU pHsgReportMcuReg contents in report register.
Output Param:
none
Return Value:
none
Usage:
in XTMP simulatin, model call this function to update Hsg status and pool
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL DSG_UpdateHsgReg(pHSG_REPORT_MCU pHsgReportMcuReg, U8 McuId)
{
    BOOL bFlag = FALSE;
    HSG_REPORT_MCU tHsgReportMcu;
    
    if(MCU0_ID == McuId)
    {
        Comm_ReadReg((U32)(&rHsgReportMcu0), 1, (U32 *)(&tHsgReportMcu));
    }
    if(MCU1_ID == McuId)
    {
        Comm_ReadReg((U32)(&rHsgReportMcu1), 1, (U32 *)(&tHsgReportMcu));
    }
    if(MCU2_ID == McuId)
    {
        Comm_ReadReg((U32)(&rHsgReportMcu2), 1, (U32 *)(&tHsgReportMcu));
    }

    //set sata DSG type0 invalid
    if(0 == pHsgReportMcuReg->bsHsgValue)
    {
        HSG_SetHsgInvalid((U16)pHsgReportMcuReg->bsHsgWrIndex);
        bFlag = TRUE;
    }
    //set sata DSG type0 valid
    else
    {
        HSG_SetHsgValid((U16)pHsgReportMcuReg->bsHsgWrIndex);
        bFlag = FALSE;
    }

    tHsgReportMcu.bsHsgWrEn = FALSE;

    if(MCU0_ID == McuId)
    {
        Comm_WriteReg((U32)(&rHsgReportMcu0), 1, (U32 *)(&tHsgReportMcu));
    }
    if(MCU1_ID == McuId)
    {
        Comm_WriteReg((U32)(&rHsgReportMcu1), 1, (U32 *)(&tHsgReportMcu));
    }
    if(MCU2_ID == McuId)
    {
        Comm_WriteReg((U32)(&rHsgReportMcu2), 1, (U32 *)(&tHsgReportMcu));
    }

    return bFlag;
}

/*------------------------------------------------------------------------------
Name: HsgRegWriteProcess
Description:
the detail process flow when HSG reg is writed.
Input Param:
U32 ulRegvalue:
U8 ucMcuId:
Output Param:
none
Return Value:
BOOL
FALSE:updated register contents to firmware readable.
TRUE:updated register contents to firmware not readable.
Usage:
invoke by Sata DSG CALLBACK function.
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL HsgRegWriteProcess(U32 ulRegvalue, U8 ucMcuId)
{
    pHSG_REPORT_MCU ptHsgReport;
    BOOL bFlag;

    ptHsgReport = (pHSG_REPORT_MCU)(&ulRegvalue);
    if(TRUE == ptHsgReport->bsHsgTrigger)
    {
        HSG_AllocateHsg(ucMcuId);
        bFlag = FALSE;
    }
    else if(TRUE == ptHsgReport->bsHsgWrEn)
    {
        DSG_UpdateHsgReg(ptHsgReport, ucMcuId);
        bFlag = FALSE;
    }
    else
    {
        bFlag = TRUE;
    }

    return bFlag;
}

/*------------------------------------------------------------------------------
Name: HsgRegWrite
Description:
Sata DSG CALLBACK function
Input Param:
U32 regaddr:
U32 regvalue:
U32 nsize:
Output Param:
none
Return Value:
BOOL
FALSE:updated register contents to firmware readable.
TRUE:updated register contents to firmware not readable.
Usage:
in XTMP simulatin, model call this function when HSG and Sata DSG related registers be written.
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL HsgRegWrite(U32 regaddr, U32 regvalue, U32 nsize)
{
    BOOL bFlag;
    
    if(regaddr >= (U32)(&rHsgReportMcu0) && regaddr < ((U32)(&rHsgReportMcu0)+sizeof(U32)))
    {
        bFlag = HsgRegWriteProcess(regvalue, MCU0_ID);
    }
    if(regaddr >= (U32)(&rHsgReportMcu1) && regaddr < ((U32)(&rHsgReportMcu1)+sizeof(U32)))
    {
       bFlag = HsgRegWriteProcess(regvalue, MCU1_ID);
    }
    if(regaddr >= (U32)(&rHsgReportMcu2) && regaddr < ((U32)(&rHsgReportMcu2)+sizeof(U32)))
    {
       bFlag = HsgRegWriteProcess(regvalue, MCU2_ID);
    }
    return bFlag;
}