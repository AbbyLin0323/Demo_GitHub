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
* File Name    : L0_NVMeDataIO.c
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2014.12.18
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_Inc.h"
#include "COM_Memory.h"
#include "HAL_HSG.h"
#include "HAL_SGE.h"
#include "L0_Interface.h"
#include "L0_NVMe.h"
#include "L0_NVMeDataIO.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
//#define PRP_XFER_ABILITY_LEN(pPrpPtr)  (HPAGE_SIZE - ((pPrpPtr)->Offset<<2))

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern void DBG_PrintDSG(U32 ulDSGID);
extern void DBG_PrintHSG(U32 ulHSGID);
extern GLOBAL U32 g_ulSubsysNum;
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/


GLOBAL volatile U32 *g_pL0DataBuffBusy;

LOCAL U32 L0BuildDataDSG(PL0_NVMeDATAREQ pHostDataReq);
LOCAL U32 L0BuildDataHSG(PL0_NVMeDATAREQ pHostDataReq);
LOCAL U32 L0TransPRPtoHSG(const L0_NVMeDATAREQ *pHostDataReq, U16 usCurrHSGId, U16 usNextHSGId);
LOCAL U32 L0BuildDataDRQDWQ(const L0_NVMeDATAREQ *pHostDataReq);

void L0_HostDataXferInit(void)
{
    HAL_NormalDsgInit();
    HAL_HsgInit();
    HAL_DrqInit();
    HAL_DwqInit();

    L0_ClearDataBuff();

    return;
}

U32 L0_CreateHostDataRequest(PL0_NVMeDATAREQ pHostDataReq)
{
    U32 ulFinished = FALSE;
    U32 ulProgress = pHostDataReq->ucReqBuildStatus;

    switch (ulProgress)
    {
        case NVME_DATAIO_BUILDDSG:
            if (TRUE == L0BuildDataDSG(pHostDataReq))
            {
                ulProgress = NVME_DATAIO_BUILDHSG;
            }
            else
            {
                break;
            }

            //break;    /* mask this break for process this request maybe at one time. [haven] */
        /*lint -e(825)*/
        case NVME_DATAIO_BUILDHSG:
            if (TRUE == L0BuildDataHSG(pHostDataReq))
            {
                ulProgress = NVME_DATAIO_BUILDDRQ;
            }
            else
            {
                break;
            }

            //break;    /* mask this break for process this request maybe at one time. [haven] */
        /*lint -e(825)*/
        case NVME_DATAIO_BUILDDRQ:
            if (TRUE == L0BuildDataDRQDWQ(pHostDataReq))
            {
                ulProgress = NVME_DATAIO_BUILDEND;
                ulFinished = TRUE;
            }

            break;

        default:
            DBG_Getch();
    }

    if (ulProgress != pHostDataReq->ucReqBuildStatus)
    {
        pHostDataReq->ucReqBuildStatus = ulProgress;
    }

    return ulFinished;
}

void L0_ClearDataBuff(void)
{
    *g_pL0DataBuffBusy = L0_HOSTDATAXFER_FINISHED;
}

void L0_SetDataBuffBusy(void)
{
    *g_pL0DataBuffBusy = L0_HOSTDATAXFER_BUSY;
}

BOOL L0_DataXferDone(void)
{
    return (L0_HOSTDATAXFER_FINISHED == *g_pL0DataBuffBusy);
}


BOOL L0_NVMeXferData(PCB_MGR pSlot)
{
    static L0_NVMeDATAREQ tLocalDataReq;
    BOOL bFinish = FALSE;

    switch (pSlot->DataXferState)
    {
        case NVME_DATAXFER_START:
            /* Sets data transfer initial parameters. */
            COM_MemZero((U32*)&tLocalDataReq, sizeof(L0_NVMeDATAREQ)/sizeof(U32));

            tLocalDataReq.ulDRAMBuffStart = pSlot->DataAddr;
            tLocalDataReq.ulByteLen = pSlot->TotalRemainingBytes;
            tLocalDataReq.ucHCmdSlt = pSlot->SlotNum;
            tLocalDataReq.ucDataIsC2H = pSlot->IsWriteDir;
            tLocalDataReq.usPrevHSGId = INVALID_4F;
            tLocalDataReq.ucReqBuildStatus = NVME_DATAIO_BUILDDSG;

            L0_SetDataBuffBusy();
            pSlot->DataXferState = NVME_DATAXFER_SETUP;

        case NVME_DATAXFER_SETUP: 
            if (TRUE == L0_CreateHostDataRequest(&tLocalDataReq))
            {
                pSlot->DataXferState = NVME_DATAXFER_INPRGS;
            }

            break;

        case NVME_DATAXFER_INPRGS:
            if (TRUE == L0_DataXferDone())
            {
                pSlot->DataXferState = NVME_DATAXFER_DONE;
                bFinish = TRUE;
            }

            break;

        default:
            DBG_Printf("L0 Data Transfer State machine error\n");
            DBG_Getch();
    }

    return bFinish;
}



LOCAL U32 L0BuildDataDSG(PL0_NVMeDATAREQ pHostDataReq)
{
    U32 ulFinished;

    NORMAL_DSG_ENTRY *pCurDSG;
    U16 usCurDSGId;

    if (FALSE == HAL_GetNormalDsg(&usCurDSGId))
    {
        /* There is not an available DSG currently. */
        ulFinished = FALSE;
    }

    else
    {
        /* 1. Acquires a DSG entry. */
        pCurDSG = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(usCurDSGId);
        COM_MemZero((U32 *)pCurDSG, sizeof(NORMAL_DSG_ENTRY)/sizeof(U32));

        /* 2. Records current DSG ID as our first DSG. */
        pHostDataReq->usFirstDSGId = usCurDSGId;
        
        /* 3. Programs local DRAM address and length for transfer. */
        pCurDSG->bsDramAddr = (pHostDataReq->ulDRAMBuffStart - DRAM_START_ADDRESS)>>1;
        pCurDSG->bsXferByteLen = pHostDataReq->ulByteLen;

        /* 4. Programs cache status update behavior for hardware. */
#ifdef L0_DSG_CACHESTS_IN_OTFB
        pCurDSG->bsCsInDram = FALSE; /* Specifies cache status updated in OTFB. */
        pCurDSG->bsCacheStatusAddr = (U32)g_pL0DataBuffBusy - OTFB_START_ADDRESS;
#else
        pCurDSG->bsCsInDram = TRUE; /* Specifies cache status updated in DRAM. */
        pCurDSG->bsCacheStatusAddr = (U32)g_pL0DataBuffBusy - DRAM_START_ADDRESS;
#endif
        pCurDSG->bsCacheStsData = L0_HOSTDATAXFER_FINISHED;
        pCurDSG->bsCacheStsEn = TRUE;

        /* 5. Finishes the DSG programming. */
        pCurDSG->bsLast = TRUE;

        DBG_PrintDSG(usCurDSGId);
        HAL_SetNormalDsgSts(usCurDSGId, NORMAL_DSG_VALID);

        ulFinished = TRUE;
    }

    return ulFinished;
}


LOCAL U32 L0BuildDataHSG(PL0_NVMeDATAREQ pHostDataReq)
{
    U16 usCurrHSGId;
    U16 usNextHSGId;
    U16 usHSGXferLen;
    PPRP pCurPrp;

    /* 1. Attempts to acquire one HSG entry from hardware. */
    if (FALSE == HAL_GetHsg(&usCurrHSGId))
    {
        /* There is nothing to do due to lacking of HSG resource. */
        return FALSE;
    }

    if (INVALID_4F == pHostDataReq->usPrevHSGId)
    {
        /* The first HSG entry has not been allocated before.
                    Updates information for it. */
        pHostDataReq->usFirstHSGId = usCurrHSGId;
        pHostDataReq->usPrevHSGId = usCurrHSGId;
        pHostDataReq->ulHSGRemBytes = pHostDataReq->ulByteLen;
        pHostDataReq->ucCurrentPRP = 0;

        pCurPrp = (PPRP)HAL_GetLocalPrpEntryAddr(pHostDataReq->ucHCmdSlt, 0);

        if (PRP_ABILITY(pCurPrp->ulDW0) >= pHostDataReq->ulHSGRemBytes)
        {
            /* Only one HSG is needed for this command. */
            usNextHSGId = INVALID_4F;
        }

        else
        {
            /* Attempts to acquire the next HSG entry for future use. */
            if (FALSE == HAL_GetHsg(&usNextHSGId))
            {
                return FALSE;
            }
        }
    }

    else
    {
        /* Last building had been terminated due to
                    lacking of the next HSG entry. */
        usNextHSGId = usCurrHSGId;
        usCurrHSGId = pHostDataReq->usPrevHSGId;
    }

    /* 2. All prequisites are satisfied for building one HSG entry now. */
    while (TRUE)
    {
        /* Programs current HSG entry. */
        usHSGXferLen = L0TransPRPtoHSG(pHostDataReq, usCurrHSGId, usNextHSGId);

        if (INVALID_4F == usNextHSGId)
        {
            /* Just finished last HSG entry. */
            return TRUE;
        }

        else
        {
            /* Updates information for next HSG entry. */
            pHostDataReq->ucCurrentPRP++;
            pHostDataReq->ulHSGRemBytes -= usHSGXferLen;
            usCurrHSGId = usNextHSGId;
            pHostDataReq->usPrevHSGId = usCurrHSGId;

            if (pHostDataReq->ulHSGRemBytes <= HPAGE_SIZE)
            {
                /* Reached the last HSG entry. */
                usNextHSGId = INVALID_4F;
            }

            else
            {
                /* Attempts to acquire the next HSG entry for future use. */
                if (FALSE == HAL_GetHsg(&usNextHSGId))
                {
                    return FALSE;
                }
            }
        }
    }
}

LOCAL U32 L0TransPRPtoHSG(const L0_NVMeDATAREQ *pHostDataReq, U16 usCurrHSGId, U16 usNextHSGId)
{
    PPRP pCurrPrp;
    HSG_ENTRY *pCurrHSG;

    U32 ulCurrPRDXferLen;

    /* 1. Calculates memory address for host PRD and our HSG entry. */
    pCurrPrp = (PPRP)HAL_GetLocalPrpEntryAddr(pHostDataReq->ucHCmdSlt, pHostDataReq->ucCurrentPRP);
    pCurrHSG = (HSG_ENTRY *)HAL_GetHsgAddr(usCurrHSGId);

    /* 2. Fills HSG parameters. */
    /* 1) Zero-fills whole HSG entry area. */
    COM_MemZero((U32 *)pCurrHSG, sizeof(HSG_ENTRY)/sizeof(U32));

    /* 2) Target address. */
    pCurrHSG->ulHostAddrHigh = pCurrPrp->ulDW1;
    pCurrHSG->ulHostAddrLow = pCurrPrp->ulDW0;


    /* 3) Transfer length (in byte). */
    ulCurrPRDXferLen = min(PRP_ABILITY(pCurrPrp->ulDW0), pHostDataReq->ulHSGRemBytes);
    pCurrHSG->bsLength = ulCurrPRDXferLen;

    /* 4) Next HSG entry in the chain. */
    pCurrHSG->bsNextHsgId = usNextHSGId;

    /* 5) End of HSG chain flag. */
    if (INVALID_4F == usNextHSGId)
    {
        pCurrHSG->bsLast = TRUE;
    }

    /* 3. Activates the DSG entry. */
    DBG_PrintHSG(usCurrHSGId);
    HAL_SetHsgSts(usCurrHSGId, HSG_VALID);

    return ulCurrPRDXferLen;
}

LOCAL U32 L0BuildDataDRQDWQ(const L0_NVMeDATAREQ *pHostDataReq)
{
    U32 ulFinished;

    U16 usFirstHSGId, usFirstDSGId;
    U8 ucCmdSlot, ucDataDirC2H;

    ucCmdSlot = pHostDataReq->ucHCmdSlt;
    ucDataDirC2H = pHostDataReq->ucDataIsC2H;
    usFirstDSGId = pHostDataReq->usFirstDSGId;
    usFirstHSGId = pHostDataReq->usFirstHSGId;

    if (TRUE == pHostDataReq->ucDataIsC2H)
    {
        ulFinished = HAL_DrqBuildEntry(ucCmdSlot, usFirstHSGId, usFirstDSGId);
    }

    else
    {
        ulFinished = HAL_DwqBuildEntry(ucCmdSlot, usFirstHSGId, usFirstDSGId);
    }

    return ulFinished;
}



/*====================End of this file========================================*/

