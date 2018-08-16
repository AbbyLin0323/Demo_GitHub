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
#include "sim_SataDSG.h"
#include "memory_access.h"
#include "HAL_Xtensa.h"
#include <string.h>
#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#endif

#include "HAL_SataDSG.h"

extern void Comm_WriteReg(U32 addr, U32 nWords, U32 *buf);
extern void Comm_ReadReg(U32 addr, U32 nWords, U32 *buf);
extern void Comm_WriteOtfbByByte(U32 addr, U32 nBytes, U8 *buf);
extern void Comm_WriteDramByByte(U32 addr, U32 nBytes, U8 *buf);

LOCAL U16 l_usReadDsgHead;
LOCAL U16 l_usReadDsgCount;
LOCAL U16 l_usWriteDsgHead;
LOCAL U16 l_usWriteDsgCount;
LOCAL U16 l_aFreeReadDsg[SATA_TYPE0_DSG_NUM];
LOCAL U16 l_aFreeWriteDsg[SATA_TYPE1_DSG_NUM];
LOCAL BOOL l_aSataDsgValidSts[SATA_TOTAL_DSG_NUM];
//Read/Write DSG count which can be used by FW, including the one which is being shown in report register
LOCAL U8 l_ucFwAvailableReadDsgCount;
LOCAL U8 l_ucFwAvailableWriteDsgCount;

LOCAL CRITICAL_SECTION l_CriticalSataReadIndex;
LOCAL CRITICAL_SECTION l_CriticalSataWriteIndex;

LOCAL SATA_DSG * l_pSataDsgEntryBase;

/*------------------------------------------------------------------------------
Name: DSG_InitSataDsg
Description:
initialize sata DSG base address;
set type0/1 DSG pool to default status.
set sata DSG related registers to default status.
Input Param:
void
Output Param:
none
Return Value:
void
Usage:
in model init stage, call this function to initialize sata DSG;
any other sata DSG functions can be called after this function finished.
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
void DSG_InitSataDsg(void)
{
    U16 usDsgIndex;
    U32 *pHsgBaseAddr = NULL;
    U32 *pSataSts = NULL;
    SATA_DSG_REPORT tSataDsgReport;

    for (usDsgIndex = 0; usDsgIndex < SATA_TYPE0_DSG_NUM; usDsgIndex++)
    {
        /* type0 DSG id start from 0 */
        l_aFreeReadDsg[usDsgIndex] = usDsgIndex;
    }

    for (usDsgIndex = 0; usDsgIndex < SATA_TYPE1_DSG_NUM; usDsgIndex++)
    {
        /* type1 DSG id start follow type0 last DSG id */
        l_aFreeWriteDsg[usDsgIndex] = usDsgIndex + SATA_TYPE0_DSG_NUM;
    }

    /* reset all sata DSG to invalid status */
    for (usDsgIndex = 0; usDsgIndex < SATA_TOTAL_DSG_NUM; usDsgIndex++)
    {
        l_aSataDsgValidSts[usDsgIndex] = FALSE;
    }

    *((U32 *)(&tSataDsgReport)) = 0;
    tSataDsgReport.bsSataDsgId = 0;
    tSataDsgReport.bsSataDsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rSataDsgReportRead1), 1,(U32 *)(&tSataDsgReport));

    *((U32 *)(&tSataDsgReport)) = 0;
    tSataDsgReport.bsSataDsgId = 0+SATA_TYPE0_DSG_NUM;
    tSataDsgReport.bsSataDsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rSataDsgReportWrite1), 1,(U32 *)(&tSataDsgReport));


    *((U32 *)(&tSataDsgReport)) = 0;
    tSataDsgReport.bsSataDsgId = 1;
    tSataDsgReport.bsSataDsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rSataDsgReportRead2), 1,(U32 *)(&tSataDsgReport));

    *((U32 *)(&tSataDsgReport)) = 0;
    tSataDsgReport.bsSataDsgId = 1+SATA_TYPE0_DSG_NUM;
    tSataDsgReport.bsSataDsgValidEn = TRUE;
    Comm_WriteReg((U32)(&rSataDsgReportWrite2), 1,(U32 *)(&tSataDsgReport));

    l_usReadDsgHead = 2;
    l_usReadDsgCount = SATA_TYPE0_DSG_NUM-2;

    l_usWriteDsgHead = 2;
    l_usWriteDsgCount = SATA_TYPE1_DSG_NUM-2;
    l_ucFwAvailableReadDsgCount = SATA_TYPE0_DSG_NUM;
    l_ucFwAvailableWriteDsgCount = SATA_TYPE1_DSG_NUM;
    Comm_WriteRegByByte((U32)&rAvailableReadDsgCnt, 1, &l_ucFwAvailableReadDsgCount);
    Comm_WriteRegByByte((U32)&rAvailableWriteDsgCnt, 1, &l_ucFwAvailableWriteDsgCount);

    return;
}

/*------------------------------------------------------------------------------
Name: DSG_AllocateReadDsg
Description:
allocate one sata type0 DSG from pool.
Input Param:
none
Output Param:
none
Return Value:
Bool:
0 = no DSG got;
1 = one DSG got;
Usage:
in XTMP simulatin, model call this function to get one DSG.
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_AllocateReadDsg(SATA_DSG_REPORT * pSataDsgReport)
{
    U16 usCurSataId;
    BOOL bGetDsgFlag = FALSE;

    if (0 == l_usReadDsgCount)
    {
        usCurSataId = INVALID_4F;
        bGetDsgFlag = FALSE;
        pSataDsgReport->bsSataDsgValidEn = FALSE;
    }
    else
    {
        usCurSataId = l_aFreeReadDsg[l_usReadDsgHead];
        l_usReadDsgCount--;
        l_usReadDsgHead++;
        l_usReadDsgHead %= SATA_TYPE0_DSG_NUM;

        bGetDsgFlag = TRUE;
        pSataDsgReport->bsSataDsgValidEn = TRUE;
    }

    pSataDsgReport->bsSataDsgTrigger = FALSE;
    pSataDsgReport->bsSataDsgId = usCurSataId;

    if (0 != l_ucFwAvailableReadDsgCount)
    {
        l_ucFwAvailableReadDsgCount--;
        Comm_WriteRegByByte((U32)&rAvailableReadDsgCnt, 1, &l_ucFwAvailableReadDsgCount);
    }

    return bGetDsgFlag;
}

/*------------------------------------------------------------------------------
Name: DSG_AllocateSataReadDsg2
Description:
allocate one sata type0 DSG from pool and report it on register rSataDsgReportRead2.
Input Param:
none
Output Param:
none
Return Value:
Bool:
0 = no DSG got;
1 = one DSG got;
Usage:
in XTMP simulatin, model call this function to get one DSG.
in Win simulation, this function used as stub fucntion in FW
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_AllocateSataReadDsg2(void)
{
    BOOL bGetDsgFlag = FALSE;
    SATA_DSG_REPORT tSataDsgReport;

    Comm_ReadReg((U32)(&rSataDsgReportRead2), 1, (U32 *)(&tSataDsgReport));
    EnterCriticalSection(&l_CriticalSataReadIndex);
    bGetDsgFlag = DSG_AllocateReadDsg(&tSataDsgReport);
    LeaveCriticalSection(&l_CriticalSataReadIndex);
    Comm_WriteReg((U32)(&rSataDsgReportRead2), 1, (U32 *)(&tSataDsgReport));

    return bGetDsgFlag;
}

/*------------------------------------------------------------------------------
Name: DSG_AllocateSataReadDsg2
Description:
allocate one sata type0 DSG from pool and report is on rSataDsgReportRead1.
Input Param:
none
Output Param:
none
Return Value:
Bool:
0 = no DSG got;
1 = one DSG got;
Usage:
in XTMP simulatin, model call this function to get one DSG.
in Win simulation, this function used as stub fucntion in FW
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_AllocateSataReadDsg1(void)
{
    BOOL bGetDsgFlag = FALSE;
    SATA_DSG_REPORT tSataDsgReport;

    Comm_ReadReg((U32)(&rSataDsgReportRead1), 1, (U32 *)(&tSataDsgReport));
    EnterCriticalSection(&l_CriticalSataReadIndex);
    bGetDsgFlag = DSG_AllocateReadDsg(&tSataDsgReport);
    LeaveCriticalSection(&l_CriticalSataReadIndex);
    Comm_WriteReg((U32)(&rSataDsgReportRead1), 1, (U32 *)(&tSataDsgReport));

    return bGetDsgFlag;
}

/*------------------------------------------------------------------------------
Name: DSG_AllocateWriteDsg
Description:
allocate one sata type1 DSG from pool.
Input Param:
none
Output Param:
none
Return Value:
BOOL:
0 = no DSG got;
1 = one DSG got;
other = invalid return value
Usage:
in XTMP simulatin, model call this function to get one DSG.
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_AllocateWriteDsg(SATA_DSG_REPORT * pSataDsgReport)
{
    U16 usCurSataId;
    BOOL bGetDsgFlag = FALSE;

    if (0 == l_usWriteDsgCount)
    {
        usCurSataId = INVALID_4F;
        bGetDsgFlag = FALSE;
        pSataDsgReport->bsSataDsgValidEn = FALSE;
    }
    else
    {
        usCurSataId = l_aFreeWriteDsg[l_usWriteDsgHead];
        l_usWriteDsgCount--;
        l_usWriteDsgHead++;
        l_usWriteDsgHead %= SATA_TYPE1_DSG_NUM;

        bGetDsgFlag = TRUE;
        pSataDsgReport->bsSataDsgValidEn = TRUE;
    }

    pSataDsgReport->bsSataDsgTrigger = FALSE;
    pSataDsgReport->bsSataDsgId = usCurSataId;
    
    if (0 != l_ucFwAvailableWriteDsgCount)
    {
        l_ucFwAvailableWriteDsgCount--;
        Comm_WriteRegByByte((U32)&rAvailableWriteDsgCnt, 1, &l_ucFwAvailableWriteDsgCount);
    }

    return bGetDsgFlag;
}

/*------------------------------------------------------------------------------
Name: DSG_AllocateSataWriteDsg1
Description:
allocate one sata type1 DSG from pool and report it on rSataDsgReportWrite1.
Input Param:
none
Output Param:
none
Return Value:
BOOL:
0 = no DSG got;
1 = one DSG got;
other = invalid return value
Usage:
in XTMP simulatin, model call this function to get one DSG.
in Win simulation, this function used as stub fucntion in FW
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_AllocateSataWriteDsg1(void)
{
    BOOL bGetDsgFlag = FALSE;
    SATA_DSG_REPORT tSataDsgReport;

    Comm_ReadReg((U32)(&rSataDsgReportWrite1), 1, (U32 *)(&tSataDsgReport));
    EnterCriticalSection(&l_CriticalSataWriteIndex);
    bGetDsgFlag = DSG_AllocateWriteDsg(&tSataDsgReport);
    LeaveCriticalSection(&l_CriticalSataWriteIndex);
    Comm_WriteReg((U32)(&rSataDsgReportWrite1), 1, (U32 *)(&tSataDsgReport));

    return bGetDsgFlag;
}

/*------------------------------------------------------------------------------
Name: DSG_AllocateSataWriteDsg2
Description:
allocate one sata type1 DSG from pool and report it on rSataDsgReportWrite2.
Input Param:
none
Output Param:
none
Return Value:
BOOL:
0 = no DSG got;
1 = one DSG got;
other = invalid return value
Usage:
in XTMP simulatin, model call this function to get one DSG.
in Win simulation, this function used as stub fucntion in FW
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_AllocateSataWriteDsg2(void)
{
    BOOL bGetDsgFlag = FALSE;
    SATA_DSG_REPORT tSataDsgReport;

    Comm_ReadReg((U32)(&rSataDsgReportWrite2), 1, (U32 *)(&tSataDsgReport));
    EnterCriticalSection(&l_CriticalSataWriteIndex);
    bGetDsgFlag = DSG_AllocateWriteDsg(&tSataDsgReport);
    LeaveCriticalSection(&l_CriticalSataWriteIndex);
    Comm_WriteReg((U32)(&rSataDsgReportWrite2), 1, (U32 *)(&tSataDsgReport));

    return bGetDsgFlag;
}
/*------------------------------------------------------------------------------
Name: DSG_SetSataDsgValid
Description:
set one sata DSG to valid status.
Input Param:
U16 DsgId: sata DSG id
Output Param:
none
Return Value:
void
Usage:
in XTMP simulatin, SDC model call this function to set one sata DSG to valid.
in Win simulation, this function used as stub fucntion in FW
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
void DSG_SetSataDsgValid(U16 DsgId)
{
    /* debug checking: valid status can be set only once by firmware */
    if(TRUE == l_aSataDsgValidSts[DsgId])
    {
        DBG_Getch();
    }
    l_aSataDsgValidSts[DsgId] = TRUE;

    return;
}

/*------------------------------------------------------------------------------
Name: DSG_SetSataDsgInvalid
Description:
set one sata DSG to invalid status.
in winsim platform update DSG pool at the same time.
Input Param:
U16 DsgId: sata DSG id
Output Param:
none
Return Value:
void
Usage:
in XTMP simulatin, SDC model call this function to set one sata DSG to invalid.
in Win simulation, this function used as stub fucntion in FW
History:
20130912  Tobey   created
------------------------------------------------------------------------------*/
void DSG_SetSataDsgInvalid(U16 DsgId)
{
    /* debug checking: invalid status can be set only once by firmware */
    if(FALSE == l_aSataDsgValidSts[DsgId])
    {
        DBG_Getch();
    }
    if(DsgId < SATA_TYPE0_DSG_NUM)
    {
        EnterCriticalSection(&l_CriticalSataReadIndex);
        l_aSataDsgValidSts[DsgId] = FALSE;
        l_aFreeReadDsg[(l_usReadDsgHead + l_usReadDsgCount)%SATA_TYPE0_DSG_NUM] = DsgId;
        l_usReadDsgCount++;
        l_ucFwAvailableReadDsgCount++;
        Comm_WriteRegByByte((U32)&rAvailableReadDsgCnt, 1, &l_ucFwAvailableReadDsgCount);

        LeaveCriticalSection(&l_CriticalSataReadIndex);
    }
    else
    {
        EnterCriticalSection(&l_CriticalSataWriteIndex);
        l_aSataDsgValidSts[DsgId] = FALSE;
        l_aFreeWriteDsg[(l_usWriteDsgHead + l_usWriteDsgCount)%SATA_TYPE1_DSG_NUM] = DsgId;
        l_usWriteDsgCount++;

        l_ucFwAvailableWriteDsgCount++;
        Comm_WriteRegByByte((U32)&rAvailableWriteDsgCnt, 1, &l_ucFwAvailableWriteDsgCount);

        LeaveCriticalSection(&l_CriticalSataWriteIndex);
    }

    return;
}

/*------------------------------------------------------------------------------
Name: DSG_IsSataDsgValid
Description:
check sata DSG is valid or not.
Input Param:
U16 DsgId: sata DSG id
Output Param:
none
Return Value:
BOOL: TRUE = DSG is valid; FALSE = DSG is not valid
Usage:
in XTMP/Win simulatin, SDC model call this function to check sata DSG is valid or not.
History:
20130902    Gavin   created
------------------------------------------------------------------------------*/
BOOL DSG_IsSataDsgValid(U16 DsgId)
{
    return l_aSataDsgValidSts[DsgId];
}

/*------------------------------------------------------------------------------
Name: DSG_ReleaseSataDsg
Description:
release one sata DSG to pool, change DSG status to invalid;
if cache status enabled in DSG, it will be updated.
Input Param:
U16 DsgId: sata DSG id
Output Param:
none
Return Value:
none
Usage:
in XTMP/Win simulatin, SDC model call this function to release sata DSG.
in Win simulation, when FW do error handling, this function used as stub fucntion in FW
History:
20130828    Gavin   created
20141218    Gavin   add debug check. add comment for cache status updating
------------------------------------------------------------------------------*/
void DSG_ReleaseSataDsg(U16 DsgId)
{
    /* We do not update cache status here, because it was updated by SDC model.
       However, in HW design, cache status may be updated by SGE.
    */

    /* debug checking: only valid DSG can be released */
    if(FALSE == l_aSataDsgValidSts[DsgId])
    {
        DBG_Getch();
    }

    DSG_SetSataDsgInvalid(DsgId);
    return;
}

/*------------------------------------------------------------------------------
Name: DSG_FetchSataDsg
Description:
get contents of sata DSG.
Input Param:
U16 DsgId: sata DSG id
Output Param:
SATA_DSG *PSataDsg: pointer to sata DSG entry
Return Value:
void
Usage:
in XTMP/Win simulatin, model call this function to get contents of sata DSG
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
void DSG_FetchSataDsg(U16 DsgId, SATA_DSG *PSataDsg)
{
    SATA_DSG *pSataDsgTemp;
    pSataDsgTemp = &l_pSataDsgEntryBase[DsgId];
    memcpy((void *)PSataDsg, (void *)pSataDsgTemp, sizeof(SATA_DSG));

    return;
}


/*------------------------------------------------------------------------------
Name: Comm_SataDsgModelInit
Description:
Initialize SataDsg model.
Input Param:
none
Output Param:
none
Return Value:
void
Usage:
in XTMP/Win simulatin, model call this function to Initialize SataDsg model.
History:
20130828    Gavin   created
------------------------------------------------------------------------------*/
void Comm_SataDsgModelInit(void)
{
    InitializeCriticalSection(&l_CriticalSataReadIndex);
    InitializeCriticalSection(&l_CriticalSataWriteIndex);

    DSG_InitSataDsg();
#ifdef SIM
    l_pSataDsgEntryBase = (SATA_DSG *)SATA_DSG_BASE;
#else
    l_pSataDsgEntryBase = (SATA_DSG *)GetVirtualAddrInLocalMem(SATA_DSG_BASE);
#endif
}

#if defined(SIM_XTENSA)
/*------------------------------------------------------------------------------
Name: DSG_UpdateSataReadReg2
Description:
update sata dsg status and type0 pool
Input Param:
pHSG_REPORT_MCU pHsgReportMcuReg contents in report register.
Output Param:
none
Return Value:
none
Usage:
in XTMP simulatin, model call this function to update dsg status and pool
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL DSG_UpdateSataReadReg2(pSATA_DSG_REPORT pSataDsgReportMcuReg)
{
    BOOL bFlag = FALSE;
    SATA_DSG_REPORT tSataReport;

    Comm_ReadReg((U32)(&rSataDsgReportRead2), 1, (U32 *)(&tSataReport));
    //set sata DSG type0 invalid
    if(0 == pSataDsgReportMcuReg->bsSataDsgValue)
    {
        DSG_SetSataDsgInvalid((U8)pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = TRUE;
    }
    //set sata DSG type0 valid
    else
    {
        DSG_SetSataDsgValid((U8)pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = FALSE;
    }

    tSataReport.bsSataDsgWrEn = FALSE;
    Comm_WriteReg((U32)(&rSataDsgReportRead2), 1, (U32 *)(&tSataReport));

    return bFlag;
}

/*------------------------------------------------------------------------------
Name: DSG_UpdateSataReadReg1
Description:
update sata dsg status and type0 pool
Input Param:
pHSG_REPORT_MCU pHsgReportMcuReg contents in report register.
Output Param:
none
Return Value:
none
Usage:
in XTMP simulatin, model call this function to update dsg status and pool
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL DSG_UpdateSataReadReg1(pSATA_DSG_REPORT pSataDsgReportMcuReg)
{
    BOOL bFlag = FALSE;
    SATA_DSG_REPORT tSataReport;

    Comm_ReadReg((U32)(&rSataDsgReportRead1), 1, (U32 *)(&tSataReport));
    //set sata DSG type0 invalid
    if(0 == pSataDsgReportMcuReg->bsSataDsgValue)
    {
        DSG_SetSataDsgInvalid((U8)pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = TRUE;
    }
    //set sata DSG type0 valid
    else
    {
        DSG_SetSataDsgValid((U8)pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = FALSE;
    }

    tSataReport.bsSataDsgWrEn = FALSE;
    Comm_WriteReg((U32)(&rSataDsgReportRead1), 1, (U32 *)(&tSataReport));

    return bFlag;
}

/*------------------------------------------------------------------------------
Name: DSG_UpdateSataWriteReg1
Description:
update sata dsg status and type1 pool
Input Param:
pHSG_REPORT_MCU pHsgReportMcuReg contents in report register.
Output Param:
none
Return Value:
none
Usage:
in XTMP simulatin, model call this function to update dsg status and pool
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL DSG_UpdateSataWriteReg1(pSATA_DSG_REPORT pSataDsgReportMcuReg)
{
    BOOL bFlag = FALSE;
    SATA_DSG_REPORT tSataReport;

    Comm_ReadReg((U32)(&rSataDsgReportWrite1), 1, (U32 *)(&tSataReport));

    //set sata DSG type1 invalid
    if(0 == pSataDsgReportMcuReg->bsSataDsgValue)
    {
        DSG_SetSataDsgInvalid(pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = TRUE;
    }
    //set sata DSG type1 valid
    else
    {
        DSG_SetSataDsgValid(pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = FALSE;
    }

    tSataReport.bsSataDsgWrEn = FALSE;
    Comm_WriteReg((U32)(&rSataDsgReportWrite1), 1, (U32 *)(&tSataReport));

    return bFlag;
}


/*------------------------------------------------------------------------------
Name: DSG_UpdateSataWriteReg2
Description:
update sata dsg status and type1 pool
Input Param:
pHSG_REPORT_MCU pHsgReportMcuReg contents in report register.
Output Param:
none
Return Value:
none
Usage:
in XTMP simulatin, model call this function to update dsg status and pool
History:
20131015    TobeyTan   created
------------------------------------------------------------------------------*/
BOOL DSG_UpdateSataWriteReg2(pSATA_DSG_REPORT pSataDsgReportMcuReg)
{
    BOOL bFlag = FALSE;
    SATA_DSG_REPORT tSataReport;

    Comm_ReadReg((U32)(&rSataDsgReportWrite2), 1, (U32 *)(&tSataReport));

    //set sata DSG type1 invalid
    if(0 == pSataDsgReportMcuReg->bsSataDsgValue)
    {
        DSG_SetSataDsgInvalid(pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = TRUE;
    }
    //set sata DSG type1 valid
    else
    {
        DSG_SetSataDsgValid(pSataDsgReportMcuReg->bsSataDsgWrIndex);
        bFlag = FALSE;
    }

    tSataReport.bsSataDsgWrEn = FALSE;
    Comm_WriteReg((U32)(&rSataDsgReportWrite2), 1, (U32 *)(&tSataReport));

    return bFlag;
}


/*------------------------------------------------------------------------------
Name: SataHsgRegWrite
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
BOOL SataDsgRegWrite(U32 regaddr, U32 regvalue, U32 nsize)
{
    pSATA_DSG_REPORT ptSataDsgReport;
    BOOL bFlag;
    //Read DSG Req
    if(regaddr >= (U32)(&rSataDsgReportRead1) && regaddr < ((U32)(&rSataDsgReportRead1)+sizeof(U32)))
    {
        ptSataDsgReport = (pSATA_DSG_REPORT)(&regvalue);
        if(TRUE == ptSataDsgReport->bsSataDsgTrigger)
        {
             DSG_AllocateSataReadDsg1();

             bFlag = FALSE;
        }
        else if(TRUE == ptSataDsgReport->bsSataDsgWrEn)
        {

            DSG_UpdateSataReadReg1(ptSataDsgReport);

            bFlag = FALSE;
        }
        else
        {
            bFlag = TRUE;
        }
    }
    if(regaddr >= (U32)(&rSataDsgReportRead2) && regaddr < ((U32)(&rSataDsgReportRead2)+sizeof(U32)))
    {
        ptSataDsgReport = (pSATA_DSG_REPORT)(&regvalue);
        if(TRUE == ptSataDsgReport->bsSataDsgTrigger)
        {
             DSG_AllocateSataReadDsg2();

             bFlag = FALSE;
        }
        else if(TRUE == ptSataDsgReport->bsSataDsgWrEn)
        {

            DSG_UpdateSataReadReg2(ptSataDsgReport);

            bFlag = FALSE;
        }
        else
        {
            bFlag = TRUE;
        }
    }
    //Write DSG Req
    if(regaddr >= (U32)(&rSataDsgReportWrite1) && regaddr < ((U32)(&rSataDsgReportWrite1)+sizeof(U32)))
    {
        ptSataDsgReport = (pSATA_DSG_REPORT)(&regvalue);
        if(TRUE == ptSataDsgReport->bsSataDsgTrigger)
        {

            DSG_AllocateSataWriteDsg1();

            bFlag = FALSE;
        }
        else if(TRUE == ptSataDsgReport->bsSataDsgWrEn)
        {
            //SATA_DSG_MODE
            DSG_UpdateSataWriteReg1(ptSataDsgReport);

            bFlag = FALSE;
        }
        else
        {
            bFlag = TRUE;
        }
    }
    if(regaddr >= (U32)(&rSataDsgReportWrite2) && regaddr < ((U32)(&rSataDsgReportWrite2)+sizeof(U32)))
    {
        ptSataDsgReport = (pSATA_DSG_REPORT)(&regvalue);
        if(TRUE == ptSataDsgReport->bsSataDsgTrigger)
        {

            DSG_AllocateSataWriteDsg2();

            bFlag = FALSE;
        }
        else if(TRUE == ptSataDsgReport->bsSataDsgWrEn)
        {
            //SATA_DSG_MODE
            DSG_UpdateSataWriteReg2(ptSataDsgReport);

            bFlag = FALSE;
        }
        else
        {
            bFlag = TRUE;
        }
    }
    return bFlag;
}
#endif
