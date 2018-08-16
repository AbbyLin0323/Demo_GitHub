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
Filename    : HAL_HwDebug.c
Version     : Ver 1.0
Author      : Gavin
Date        : 20140729
Description : this file implement recording HW debug information by command tag.
Others      :
Modify      :
20140729     gavinyin     001 create file
20140911     gavinyin     002 modify it to meet coding style
20150317     gavinyin     003 add checking before tracing
*******************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_MultiCore.h"
#include "HAL_Xtensa.h"
#include "HAL_ParamTable.h"
#include "HAL_HCT.h"
#include "HAL_HSG.h"
#include "HAL_NormalDSG.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_SGE.h"
#include "HAL_HwDebug.h"

//temporary pointer for save NFCQ pointer before record SGQ information
GLOBAL MCU12_VAR_ATTR NFCQ_ENTRY *g_pNfcqForHalDebug;
//pointer to shared trace memory
LOCAL MCU12_VAR_ATTR HW_DEBUG_INFO *l_pHwDebugInfo;
/* in multi core mode, to avoid confilct, we should record SGE debug info to 
   memory which was dedicated for current MCU.
   Note: In WholeChip FW, MCU0 will not not use l_ucMyMcuIndex
*/
LOCAL MCU12_VAR_ATTR U8 l_ucMyMcuIndex;
//switch for HW debug trace function
LOCAL MCU12_VAR_ATTR BOOL l_bHwDebugTraceEn = FALSE;

#ifdef SIM
/*------------------------------------------------------------------------------
Name: SaveMemToFile
Description: 
    save HAL trace memory to file
Input Param:
    U8 *pSrcAddr: start address of trace memory
    U32 ulByteLen: byte length of trace memory
    const char *pFileName: name of the target file
Output Param:
    none
Return Value:
    void
Usage:
    Used on Windows platform only
History:
    20140812    Gavin   create function
------------------------------------------------------------------------------*/
LOCAL void SaveMemToFile(U8 *pSrcAddr, U32 ulByteLen, const char *pFileName)
{
    FILE *fp;
    U32 ulWriteByteCnt;

    fp = fopen(pFileName, "wb");//write only, binary format

    if (NULL == fp)
    {
        DBG_Printf("fopen file %s fail\n", pFileName);
        DBG_Getch();
    }
    else
    {
        ulWriteByteCnt = (U32)fwrite(pSrcAddr, 1, ulByteLen, fp);
        if (ulByteLen != ulWriteByteCnt)
        {
            DBG_Printf("fwrite file %s fail, ulByteLen = %d, ulWriteByteCnt = %d\n", pFileName, ulByteLen, ulWriteByteCnt);
            DBG_Getch();
        }

        fclose(fp);
    }

    return;
}
#endif

/*------------------------------------------------------------------------------
Name: HAL_HwDebugInit
Description: 
    init MCU's dedicated pointer to debug memory.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function in boot stage;
History:
    20140729    Gavin   created
------------------------------------------------------------------------------*/
void HAL_HwDebugInit(void)
{
    U32 ulMcuId;
    PTABLE *pPTable;

    l_pHwDebugInfo = (HW_DEBUG_INFO *)DRAM_FW_HAL_TRACE_BASE;

    ulMcuId = HAL_GetMcuId();
    if (MCU0_ID == ulMcuId)
    {
        l_ucMyMcuIndex = 0;
    }
    else
    {
        l_ucMyMcuIndex = ulMcuId - MCU1_ID;
    }

    if (MCU1_ID == ulMcuId)//only MCU1 print the information
    {        
        DBG_Printf("HAL_HwDebug DRAM offset = 0x%x\n", DRAM_FW_HAL_TRACE_BASE - DRAM_START_ADDRESS);
        DBG_Printf("HAL_HwDebug memory size = 0x%x\n", sizeof(HW_DEBUG_INFO));
    }

    pPTable = HAL_GetPTableAddr();
    l_bHwDebugTraceEn = (pPTable->tHALFeature.ulFeatureBitMap & 0x1) ? TRUE : FALSE;
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HwDebugStart
Description: 
    reset debug information of one tag to initial state.
Input Param:
    U8 ucTag: tag of host command
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function before process a new command;
History:
    20140729    Gavin   created
    20150317    Gavin   add checking before tracing
------------------------------------------------------------------------------*/
void HAL_HwDebugStart(U8 ucTag)
{
#if defined(SUB_MODEL_TEST) || defined(HAL_UNIT_TEST) || defined(L3_UNIT_TEST) || defined(MV_PERFORMANCE_TEST) || defined(HOST_SATA)
    //do nothing
#else
    U8 ucMcuGroupIndex;
    TRACE_TAG_INFO *pTraceTagInfo = &l_pHwDebugInfo->aTraceTagInfo[ucTag];

    if (FALSE == l_bHwDebugTraceEn)
    {
        return;
    }

    pTraceTagInfo->tTraceHctInfo.ulHctDescIndex = 0;
    pTraceTagInfo->tTraceHctInfo.ulFcqWbqSaveOffset = 0;

    for (ucMcuGroupIndex = 0; ucMcuGroupIndex < SGE_DESC_GROUP_CNT_MAX; ucMcuGroupIndex++)
    {
        pTraceTagInfo->aTraceSgeInfo[ucMcuGroupIndex].ulSgeDescIndex = 0;
        pTraceTagInfo->aTraceSgeInfo[ucMcuGroupIndex].ulEntrySaveOffset = 0;
        pTraceTagInfo->aTraceSgeInfo[ucMcuGroupIndex].bAllSgeChainBuilt = FALSE;
        pTraceTagInfo->aTraceSgeInfo[ucMcuGroupIndex].usSgeTotalChain = 0;
        pTraceTagInfo->aTraceSgeInfo[ucMcuGroupIndex].bAllNfcChainBuilt = FALSE;
        pTraceTagInfo->aTraceSgeInfo[ucMcuGroupIndex].usNfcTotalChain = 0;
    }
#endif
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HwDebugTrace
Description: 
    record one FW/HW interface entry to trace memory.
Input Param:
    U8 ucTag: tag of host command
    U16 usTraceType: type of this HAL entry for record
    void *pEntry: pointer to entry which need to save
    usEntryId: chain count (usTraceType = RCD_SGE_CHAIN/RCD_NFC_CHAIN);
               entry id (usTraceType = others).
    NFCQ_ENTRY *pNfcqEntry: pointer to NFCQ entry. used for SGQ only
Output Param:
    none
Return Value:
    void
Usage:
    used in AHCI WholeChip/Ramdisk mode;
History:
    20140729    Gavin   created
    20141029    Gavin   disable for MixVector to avoid save area overflow.
    20150317    Gavin   add checking before tracing
------------------------------------------------------------------------------*/
void HAL_HwDebugTrace(U8 ucTag, U16 usTraceType, void *pEntry, U16 usEntryId, NFCQ_ENTRY *pNfcqEntry)
{
#if defined(SUB_MODEL_TEST) || defined(HAL_UNIT_TEST) || defined(L3_UNIT_TEST) || defined(MV_PERFORMANCE_TEST) || defined(HOST_SATA) || defined(MIX_VECTOR)
    //do nothing
#else
    U32 ulDescIndex;
    U32 ulEntrySaveOffset;
    HSG_ENTRY *pHsgEntry;
    NORMAL_DSG_ENTRY *pDsgEntry;
    TRACE_HCT_INFO *pTraceHctInfo;
    TRACE_SGE_INFO *pTraceSgeInfo;
    TRACE_TAG_INFO *pTraceTagInfo = &l_pHwDebugInfo->aTraceTagInfo[ucTag];

    if (FALSE == l_bHwDebugTraceEn)
    {
        return;
    }

    switch (usTraceType)
    {
        case RCD_FCQ:
        case RCD_WBQ:
            pTraceHctInfo = &pTraceTagInfo->tTraceHctInfo;
            
            /* get current descriptor index and save area offset */
            ulDescIndex = pTraceHctInfo->ulHctDescIndex;
            ulEntrySaveOffset = pTraceHctInfo->ulFcqWbqSaveOffset;

            /* add one descriptor */
            pTraceHctInfo->aHctDesc[ulDescIndex].eEntryType = usTraceType;
            pTraceHctInfo->aHctDesc[ulDescIndex].usEntryId = usEntryId;

            //record FCQ/WBQ's location in save area
            pTraceHctInfo->aHctDesc[ulDescIndex].ulFcqWbqEntryOffset = ulEntrySaveOffset;

            /* back-up FCQ/WBQ */
            COM_MemCpy((U32 *)&pTraceHctInfo->ucFcqWbqBuff[ulEntrySaveOffset],
                        (U32 *)pEntry, sizeof(HCT_FCQ_WBQ)/4);

            /* update descriptor index and save area offset for next calling */
            pTraceHctInfo->ulHctDescIndex++;
            pTraceHctInfo->ulFcqWbqSaveOffset += sizeof(HCT_FCQ_WBQ);
            
            break;
            
        case RCD_DRQ:
        case RCD_DWQ:
        case RCD_SGQ_R:
        case RCD_SGQ_W:
            if (MCU0_ID == HAL_GetMcuId())
            {
                //MCU0 has its own DRQ/DWQ to process non-media access, we do not record DRQ/DWQ for that usage
                return;
            }
            pTraceSgeInfo = &pTraceTagInfo->aTraceSgeInfo[l_ucMyMcuIndex];
            
            /* get current descriptor index and save area offset */
            ulDescIndex = pTraceSgeInfo->ulSgeDescIndex;
            ulEntrySaveOffset = pTraceSgeInfo->ulEntrySaveOffset;

            /* add one descriptor */
            pTraceSgeInfo->aSgeDesc[ulDescIndex].eEntryType = usTraceType;
            pTraceSgeInfo->aSgeDesc[ulDescIndex].usEntryId = usEntryId;
            COM_MemCpy((U32 *)&pTraceSgeInfo->aSgeDesc[ulDescIndex].tSgeEntry,
                        (U32 *)pEntry, sizeof(SGE_ENTRY)/4);

            //record first HSG's location in save area
            pTraceSgeInfo->aSgeDesc[ulDescIndex].ulHsgEntryOffset = ulEntrySaveOffset;

            //back-up all HSG in chain
            pHsgEntry = (HSG_ENTRY *)HAL_GetHsgAddr(((SGE_ENTRY *)pEntry)->bsHsgPtr);
            do
            {
                COM_MemCpy((U32 *)&pTraceSgeInfo->ucEntryBuff[ulEntrySaveOffset],
                        (U32 *)pHsgEntry, sizeof(HSG_ENTRY)/4);
                ulEntrySaveOffset += sizeof(HSG_ENTRY);

                if (TRUE == pHsgEntry->bsLast)
                {
                    break;//jump out of "while(1)" loop
                }
                else
                {
                    pHsgEntry = (HSG_ENTRY *)HAL_GetHsgAddr(pHsgEntry->bsNextHsgId);
                }
            }while(1);

            if (RCD_DRQ == usTraceType || RCD_DWQ == usTraceType)
            {
                //record first DSG's location in save area
                pTraceSgeInfo->aSgeDesc[ulDescIndex].ulDsgEntryOffset = ulEntrySaveOffset;

                //back-up all DSG for DRQ/DWQ
                pDsgEntry = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(((SGE_ENTRY *)pEntry)->bsDsgPtr);
                do
                {
                    COM_MemCpy((U32 *)&pTraceSgeInfo->ucEntryBuff[ulEntrySaveOffset],
                            (U32 *)pDsgEntry, sizeof(NORMAL_DSG_ENTRY)/4);
                    ulEntrySaveOffset += sizeof(NORMAL_DSG_ENTRY);

                    if (TRUE == pDsgEntry->bsLast)
                    {
                        break;//jump out of "while(1)" loop
                    }
                    else
                    {
                        pDsgEntry = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(pDsgEntry->bsNextDsgId);
                    }
                }while(1);
            }
            else//RCD_SGQ_R or RCD_SGQ_W
            {
                //record NFCQ's location in save area
                pTraceSgeInfo->aSgeDesc[ulDescIndex].ulNfcqEntryOffset = ulEntrySaveOffset;

                //back-up NFCQ to save area
                COM_MemCpy((U32 *)&pTraceSgeInfo->ucEntryBuff[ulEntrySaveOffset],
                            (U32 *)pNfcqEntry, sizeof(NFCQ_ENTRY)/4);
                ulEntrySaveOffset += sizeof(NFCQ_ENTRY);
            }

            /* update descriptor index and save area offset for next calling */
            pTraceSgeInfo->ulSgeDescIndex++;
            pTraceSgeInfo->ulEntrySaveOffset = ulEntrySaveOffset;
            
            //protect overflow case
            if (pTraceSgeInfo->ulSgeDescIndex >= TRACE_SGE_ENTRY_MAX_CNT)
            {
                pTraceSgeInfo->ulSgeDescIndex = 0;
                pTraceSgeInfo->ulEntrySaveOffset = 0; // round back to the beginning of save area
            }

            break;
            
        case RCD_SGE_CHAIN:
            pTraceSgeInfo = &pTraceTagInfo->aTraceSgeInfo[l_ucMyMcuIndex];
            pTraceSgeInfo->usSgeTotalChain = usEntryId;
            pTraceSgeInfo->bAllSgeChainBuilt = TRUE;

            break;

        case RCD_NFC_CHAIN:
            pTraceSgeInfo = &pTraceTagInfo->aTraceSgeInfo[l_ucMyMcuIndex];
            pTraceSgeInfo->usNfcTotalChain = usEntryId;
            pTraceSgeInfo->bAllNfcChainBuilt = TRUE;

            break;
            
        default:
            //can not get here
            DBG_Getch();
    }
#endif        
    return;
}

