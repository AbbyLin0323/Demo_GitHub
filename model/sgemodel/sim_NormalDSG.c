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

Filename     : sim_DSG.c
Version      :   Ver 1.0
Date         :
Author       :  Gavin

Description:
DSG model for XTMP/Win simulation
Modification History:
20130828    Gavin   created
********************************************************************************/
#include <string.h>
#include "sim_NormalDSG.h"
#include "HAL_Xtensa.h"
#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#endif

extern void Comm_WriteReg(U32 addr, U32 nWords, U32 *buf);
extern void Comm_ReadReg(U32 addr, U32 nWords, U32 *buf);
extern void Comm_WriteOtfbByByte(U32 addr, U32 nBytes, U8 *buf);
extern void Comm_WriteDramByByte(U32 addr, U32 nBytes, U8 *buf);

LOCAL U16 l_usNormalDsgHead;
LOCAL U16 l_usNormalDsgCount;
LOCAL U16 l_aFreeNormalDsg[NORMAL_DSG_NUM];
LOCAL BOOL l_aNormalDsgValidSts[NORMAL_DSG_NUM];
LOCAL CRITICAL_SECTION l_CriticalNormalIndex;
LOCAL pDSG_REPORT_MCU l_pDsgReportMcu1 = NULL;
LOCAL pDSG_REPORT_MCU l_pDsgReportMcu2 = NULL;

LOCAL NORMAL_DSG_ENTRY * l_pNormalDsgEntryBase;
/*------------------------------------------------------------------------------
Name: DSG_InitNormalDsg
Description:
initialize normal DSG base address;
set normal DSG pool to default status.
initialize normal DSG registers to default status
Input Param:
void
Output Param:
none
Return Value:
void
Usage:
in model init stage, call this function to initialize normal DSG;
any other normal DSG functions can be called after this function finished.
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
void DSG_InitNormalDsg(void)
{
    U16 usDsgIndex;
    U32 *pDsgBaseAddr = NULL;
    U32 * pDsgSts = NULL;
    DSG_REPORT_MCU tNormalDsgReportMcu;
    U32 ulDsgBaseAddr;

    InitializeCriticalSection(&l_CriticalNormalIndex);

    for (usDsgIndex = 0; usDsgIndex < NORMAL_DSG_NUM; usDsgIndex++)
    {
        /* Normal DSG id start from 0 */
        l_aFreeNormalDsg[usDsgIndex] = usDsgIndex;

        /*initial Normal DSG sts*/
        l_aNormalDsgValidSts[usDsgIndex] = FALSE;
    }

    //Initilaze registers
    EnterCriticalSection(&l_CriticalNormalIndex);
    l_usNormalDsgHead = 3;
    l_usNormalDsgCount = NORMAL_DSG_NUM-3;
    LeaveCriticalSection(&l_CriticalNormalIndex);

    *((U32 *)(&tNormalDsgReportMcu)) = 0;
    tNormalDsgReportMcu.bsDsgId = 2;
    tNormalDsgReportMcu.bsDsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rDsgReportMcu0), 1,(U32 *)(&tNormalDsgReportMcu));


    *((U32 *)(&tNormalDsgReportMcu)) = 0;
    tNormalDsgReportMcu.bsDsgId = 0;
    tNormalDsgReportMcu.bsDsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rDsgReportMcu1), 1,(U32 *)(&tNormalDsgReportMcu));

    *((U32 *)(&tNormalDsgReportMcu)) = 0;
    tNormalDsgReportMcu.bsDsgId = 1;
    tNormalDsgReportMcu.bsDsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rDsgReportMcu2), 1,(U32 *)(&tNormalDsgReportMcu));

    ulDsgBaseAddr = (U32)NORMAL_DSG_BASE;
    Comm_WriteReg((U32)(&rDsgBaseAddr), 1,&ulDsgBaseAddr);

    return;
}

/*------------------------------------------------------------------------------
Name: DSG_AllocateNormalDsg
Description:
allocate one normal DSG from pool.
Input Param:
none
Output Param:
none
Return Value:
Bool:
0 = no DSG got;
1 = one DSG got ;
other = invalid return value
Usage:
in XTMP simulatin, model call this function to get one DSG.
in Win simulation, this function used as stub fucntion in FW
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_AllocateNormalDsg(U8 McuId)
{
    U16 usCurNormalDsgId;
    U8 ucGetDsgCnt = 0;
    DSG_REPORT_MCU tNormalDsgReport;


    if(MCU0_ID == McuId)
    {
        Comm_ReadReg((U32)(&rDsgReportMcu0), 1, (U32 *)(&tNormalDsgReport));
    }

    if(MCU1_ID == McuId)
    {
        Comm_ReadReg((U32)(&rDsgReportMcu1), 1, (U32 *)(&tNormalDsgReport));
    }
    if(MCU2_ID == McuId)
    {
        Comm_ReadReg((U32)(&rDsgReportMcu2), 1, (U32 *)(&tNormalDsgReport));
    }

    EnterCriticalSection(&l_CriticalNormalIndex);
    if (0 == l_usNormalDsgCount)
    {
        usCurNormalDsgId = INVALID_4F;
        ucGetDsgCnt = 0;
        tNormalDsgReport.bsDsgValidEn = FALSE;
    }
    else
    {
        usCurNormalDsgId = l_aFreeNormalDsg[l_usNormalDsgHead];
        l_usNormalDsgCount--;
        l_usNormalDsgHead++;
        l_usNormalDsgHead %= NORMAL_DSG_NUM;

        ucGetDsgCnt = 1;
        tNormalDsgReport.bsDsgValidEn = TRUE;
    }
    LeaveCriticalSection(&l_CriticalNormalIndex);

    tNormalDsgReport.bsDsgTrigger = FALSE;
    tNormalDsgReport.bsDsgId = usCurNormalDsgId;
    
    if(MCU0_ID == McuId)
    {
        Comm_WriteReg((U32)(&rDsgReportMcu0), 1, (U32 *)(&tNormalDsgReport));
    }

    if(MCU1_ID == McuId)
    {
        Comm_WriteReg((U32)(&rDsgReportMcu1), 1, (U32 *)(&tNormalDsgReport));
    }
    if(MCU2_ID == McuId)
    {
        Comm_WriteReg((U32)(&rDsgReportMcu2), 1, (U32 *)(&tNormalDsgReport));
    }
    return ucGetDsgCnt;
}

/*------------------------------------------------------------------------------
Name: DSG_SetNormalDsgValid
Description:
set one Normal DSG to valid status.
Input Param:
U16 DsgId: Normal DSG id
Output Param:
none
Return Value:
void
Usage:
in XTMP simulatin, NFC model call this function to set one sata DSG to valid.
in Win simulation, this function used as stub fucntion in FW
History:
20130912    Tobey   created
------------------------------------------------------------------------------*/
void DSG_SetNormalDsgValid(U16 DsgId)
{
    if(TRUE == l_aNormalDsgValidSts[DsgId])
    {
        DBG_Getch();
    }

    EnterCriticalSection(&l_CriticalNormalIndex);
    l_aNormalDsgValidSts[DsgId] = TRUE;
    LeaveCriticalSection(&l_CriticalNormalIndex);

    return;
}

/*------------------------------------------------------------------------------
Name: DSG_SetNormalDsgInvalid
Description:
set one Normal DSG to invalid status.
in winsim add released dsg into pool at the same time.
Input Param:
U16 DsgId: Normal DSG id
Output Param:
none
Return Value:
void
Usage:
in XTMP simulatin, NFC model call this function to set one sata DSG to invalid.
in Win simulation, this function used as stub fucntion in FW
History:
20130912    Tobey   created
------------------------------------------------------------------------------*/
void DSG_SetNormalDsgInvalid(U16 DsgId)
{
    if(FALSE == l_aNormalDsgValidSts[DsgId])
    {
        DBG_Getch();
    }

    EnterCriticalSection(&l_CriticalNormalIndex);
    l_aNormalDsgValidSts[DsgId] = FALSE;
    l_aFreeNormalDsg[(l_usNormalDsgCount+l_usNormalDsgHead)%NORMAL_DSG_NUM] = DsgId;
    l_usNormalDsgCount++;
    LeaveCriticalSection(&l_CriticalNormalIndex);
    
    return;
}

/*------------------------------------------------------------------------------
Name: DSG_IsNormalDsgValid
Description:
check Normal DSG is valid or not.
Input Param:
U16 DsgId: Normal DSG id
Output Param:
none
Return Value:
BOOL: TRUE = DSG is valid; FALSE = DSG is not valid
Usage:
in XTMP/Win simulatin, NFC model call this function to check sata DSG is valid or not.
History:
20130912    Tobey   created
------------------------------------------------------------------------------*/
BOOL DSG_IsNormalDsgValid(U16 DsgId)
{
    return l_aNormalDsgValidSts[DsgId];
}

/*------------------------------------------------------------------------------
Name: DSG_ReleaseNormalDsg
Description:
release one normal DSG to pool, if cache status enabled in DSG, it will be updated.
Input Param:
U16 DsgId: normal DSG id
Output Param:
none
Return Value:
none
Usage:
in XTMP/Win simulatin, model call this function to release normal DSG.
in Win simulation, when FW do error handling, this function used as stub fucntion in FW
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
void DSG_ReleaseNormalDsg(U16 DsgId)
{
    NORMAL_DSG_ENTRY tNormalDsg;
    U8 ulCacheStsData;

    /* update cache status here */
    DSG_FetchNormalDsg(DsgId, &tNormalDsg);
    if (TRUE == tNormalDsg.bsCacheStsEn)
    {
        ulCacheStsData = (U8)(tNormalDsg.bsCacheStsData);
        if(FALSE == tNormalDsg.bsCsInDram)
        {
            Comm_WriteOtfbByByte(tNormalDsg.bsCacheStatusAddr, 1, &ulCacheStsData);
        }
        else
        {
            //Comm_WriteDram(tNormalDsg.CacheStatusAddr, 1, &ulCacheStsData);
            Comm_WriteDramByByte((U32)tNormalDsg.bsCacheStatusAddr, 1, &ulCacheStsData);
        }
    }

    DSG_SetNormalDsgInvalid(DsgId);

    return;
}

/*------------------------------------------------------------------------------
Name: DSG_FetchNormalDsg
Description:
get contents of normal DSG.
Input Param:
U16 DsgId: normal DSG id
Output Param:
NORMAL_DSG_ENTRY *PNormalDsg: pointer to normal DSG entry
Return Value:
none
Usage:
in XTMP/Win simulatin, model call this function to get contents of normal DSG
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
void DSG_FetchNormalDsg(U16 DsgId, NORMAL_DSG_ENTRY *PNormalDsg)
{
    NORMAL_DSG_ENTRY * pNorDsgEnty;
    pNorDsgEnty = &l_pNormalDsgEntryBase[DsgId];
    memcpy((void *)PNormalDsg, (void *)pNorDsgEnty, sizeof(NORMAL_DSG_ENTRY));
}

/*------------------------------------------------------------------------------
Name: DSG_UpdateNormalDsgReg
Description:
update normal dsg status and pool
Input Param:
pDSG_REPORT_MCU PDsgReportMcuReg contents in report register.
Output Param:
none
Return Value:
none
Usage:
in XTMP simulatin, model call this function to update dsg status and pool
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL DSG_UpdateNormalDsgReg(pDSG_REPORT_MCU PDsgReportMcuReg, U8 McuId)
{
    BOOL bFlag = FALSE;
    //U16 usTailPtr;
    U16 usCurDsg = 0;
    U16 usNextDsg = 0;
    DSG_REPORT_MCU tNormalDsgReportMcu;
    
    if(MCU0_ID == McuId)
    {
        Comm_ReadReg((U32)(&rDsgReportMcu0), 1, (U32 *)(&tNormalDsgReportMcu));
    }

    if(MCU1_ID == McuId)
    {
        Comm_ReadReg((U32)(&rDsgReportMcu1), 1, (U32 *)(&tNormalDsgReportMcu));
    }
    if(MCU2_ID == McuId)
    {
        Comm_ReadReg((U32)(&rDsgReportMcu2), 1, (U32 *)(&tNormalDsgReportMcu));
    }

    //set normal dsg invalid
    if(0 == PDsgReportMcuReg->bsDsgValue)
    {
        DSG_SetNormalDsgInvalid(PDsgReportMcuReg->bsDsgWrIndex);
        bFlag = TRUE;
    }
    //set normal dsg valid / dsg filled by firmware
    else
    {
        DSG_SetNormalDsgValid(PDsgReportMcuReg->bsDsgWrIndex);

        bFlag = FALSE;
    }

    tNormalDsgReportMcu.bsDsgWrEn = FALSE;
    
    if(MCU0_ID == McuId)
    {
        Comm_WriteReg((U32)(&rDsgReportMcu0), 1, (U32 *)(&tNormalDsgReportMcu));
    }

    if(MCU1_ID == McuId)
    {
        Comm_WriteReg((U32)(&rDsgReportMcu1), 1, (U32 *)(&tNormalDsgReportMcu));
    }
    if(MCU2_ID == McuId)
    {
        Comm_WriteReg((U32)(&rDsgReportMcu2), 1, (U32 *)(&tNormalDsgReportMcu));
    }

    return bFlag;
}

void Comm_NormalDsgModelInit(void)
{
    DSG_InitNormalDsg();
#ifdef SIM
    l_pNormalDsgEntryBase = (NORMAL_DSG_ENTRY *)NORMAL_DSG_BASE;
#else
    l_pNormalDsgEntryBase = (NORMAL_DSG_ENTRY *)GetVirtualAddrInLocalMem(NORMAL_DSG_BASE);
#endif
}

/*------------------------------------------------------------------------------
Name: NormalDsgRegWriteProcess
Description:
the detail process flow when NormalDsg reg is writed.
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
invoke by NormalDsg CALLBACK function.
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL NormalDsgRegWriteProcess(U32 ulRegvalue, U8 ucMcuId)
{
    BOOL bFlag = FALSE;
    pDSG_REPORT_MCU ptDsgReport;

    ptDsgReport = (pDSG_REPORT_MCU)(&ulRegvalue);
    if(TRUE == ptDsgReport->bsDsgTrigger)
    {
        DSG_AllocateNormalDsg(ucMcuId);
        bFlag = FALSE;
    }
    else if(TRUE == ptDsgReport->bsDsgWrEn)
    {
        DSG_UpdateNormalDsgReg(ptDsgReport,ucMcuId);
        bFlag = FALSE;
    }
    else
    {
        bFlag = TRUE;
    }

    return bFlag;
}
/*------------------------------------------------------------------------------
Name: NormalDsgRegWrite
Description:
Normal DSG CALLBACK function
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
in XTMP simulatin, model call this function when related registers be written.
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL NormalDsgRegWrite(U32 regaddr, U32 regvalue, U32 nsize)
{
    BOOL bFlag = FALSE;

    if(regaddr >= (U32)(&rDsgReportMcu0) && regaddr < ((U32)(&rDsgReportMcu0)+sizeof(U32)))
    {
        bFlag = NormalDsgRegWriteProcess(regvalue, MCU0_ID);
    }

    if(regaddr >= (U32)(&rDsgReportMcu1) && regaddr < ((U32)(&rDsgReportMcu1)+sizeof(U32)))
    {
        bFlag = NormalDsgRegWriteProcess(regvalue, MCU1_ID);
    }
    if(regaddr >= (U32)(&rDsgReportMcu2) && regaddr < ((U32)(&rDsgReportMcu2)+sizeof(U32)))
    {
        bFlag = NormalDsgRegWriteProcess(regvalue, MCU2_ID);
    }
    return bFlag;
}

