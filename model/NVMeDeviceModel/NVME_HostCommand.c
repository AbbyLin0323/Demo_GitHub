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
* File Name    : NVME_HostCommand.c
* Discription  :
* CreateAuthor : Haven Yang
* CreateDate   : 2014.11.3
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/

#include "model_common.h"
#include "NVMeHostModel/nvme.h"
#include "nvmeStd.h"
#include "NVME_HostCommand.h"
#include "NVME_ControllerModel.h"
#include "L0_NVME.h"
#include "COM_Memory.h"
#include "HAL_HCT.h"
#include "Sim_HCT.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern volatile HCT_CONTROL_REG *g_pHCTControlReg;
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
static NVME_CMD_TABLE g_tNVMEHostCmdTable[MAX_SLOT_NUM];


/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/

void NVME_FillHostCmdTable(U32 ulSlotNum, U32 ulHostAddrHigh, U32 ulHostAddrLow, U32 bsCST)
{
    LARGE_INTEGER ullHostAddr;
    NVMe_COMMAND *pNVMECmdHeader;
    U32 ulCurrCST;
    U8 ucViaSubCmd;
    BOOL bResult;
    /* Parameter Check 
    if(NULL == pFCQEntry)
    {
        DBG_Break();
    }
    */

    ullHostAddr.HighPart = ulHostAddrHigh;
    ullHostAddr.LowPart = ulHostAddrLow;

    pNVMECmdHeader = (NVMe_COMMAND *)ullHostAddr.QuadPart;
    ulCurrCST = HCT_GetCST((U8)ulSlotNum, NULL);
#ifdef AF_ENABLE_M
    if (g_pHCTControlReg->bAUTOFCHEN)
    {
        bResult = (g_pHCTControlReg->bsAUTOFCHCST == bsCST) ? TRUE : FALSE;
    }
    else
    {
        bResult = (ulCurrCST == bsCST) ? TRUE : FALSE;
    }
#else
    bResult = (ulCurrCST == bsCST) ? TRUE : FALSE;
#endif
    if(TRUE == bResult)
    {
        if(NULL == pNVMECmdHeader)
        {
            DBG_Break();
        }

        if(CMD_TYPE_IO == pNVMECmdHeader->CDW15)
        {
            g_tNVMEHostCmdTable[ulSlotNum].ulStartLba = (pNVMECmdHeader->CDW10) << SEC_PER_DATABLOCK_BITS;
            g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen   = (pNVMECmdHeader->CDW12 + 1) << SEC_PER_DATABLOCK_BITS;
        }
        else if(CMD_TYPE_SPECAIL == pNVMECmdHeader->CDW15)
        {
            g_tNVMEHostCmdTable[ulSlotNum].ulStartLba = 0;
            g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen   = 0;
            if(NVM_DATASET_MANAGEMENT == pNVMECmdHeader->CDW0.OPC)
            {
                g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen =(((pNVMECmdHeader->CDW10 & 0xFF) + 1)*sizeof(LBA_RANGE_ENTRY)) >> SEC_SIZE_BITS;
            }
            else if(NVM_VIA_TRACE_GETLOGINFO == pNVMECmdHeader->CDW0.OPC || NVM_VIA_TRACE_GETLOGDATA == pNVMECmdHeader->CDW0.OPC)
            {
                g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen = (pNVMECmdHeader->CDW12 + 1) << SEC_PER_DATABLOCK_BITS;
            }
        }

        if(ADMIN_IDENTIFY == pNVMECmdHeader->CDW0.OPC)
        {
            //Indentify xfer 4096 bytes
            g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen = 8;
            g_tNVMEHostCmdTable[ulSlotNum].ulStartLba = 0;
        }
        else if(ADMIN_VIA_VENDER_CMD == pNVMECmdHeader->CDW0.OPC)
        {
            ucViaSubCmd = pNVMECmdHeader->CDW12 & 0xFF;
            if(VIA_SUBCMD_MEM_READ == ucViaSubCmd || VIA_SUBCMD_MEM_WRITE == ucViaSubCmd)
            {
                g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen = pNVMECmdHeader->CDW14/SEC_SIZE;
            }
            else if(VIA_SUBCMD_FLASH_READ == ucViaSubCmd || VIA_SUBCMD_FLASH_WRITE == ucViaSubCmd || VIA_SUBCMD_FLASH_ERASE == ucViaSubCmd)
            {
                g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen = 1;
            }
            g_tNVMEHostCmdTable[ulSlotNum].ulViaSubCmd = ucViaSubCmd;
        }
        else if(ADMIN_FIRMWARE_IMAGE_DOWNLOAD == pNVMECmdHeader->CDW0.OPC)
        {
            g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen = (pNVMECmdHeader->CDW10 + 1)*sizeof(U32)/SEC_SIZE;
        }

        g_tNVMEHostCmdTable[ulSlotNum].ulCmdCode  = pNVMECmdHeader->CDW0.OPC;
        g_tNVMEHostCmdTable[ulSlotNum].PRP1 = (U32)pNVMECmdHeader->PRP1;
        g_tNVMEHostCmdTable[ulSlotNum].PRP2 = (U32)pNVMECmdHeader->PRP2;
        g_tNVMEHostCmdTable[ulSlotNum].ulXferByte = 0;
        g_tNVMEHostCmdTable[ulSlotNum].ulXferLba  = 0;
        g_tNVMEHostCmdTable[ulSlotNum].ucCmdFinishFlag  = FALSE;
        g_tNVMEHostCmdTable[ulSlotNum].Protocal = (U32)pNVMECmdHeader->CDW15;
    }
#if 0
    else if (SLOT_WAITING_PRPLIST == bsCST)
    {
        /* fill PRP list, to be continue */
    }
    else
    {
        /* do nothing */
    }
#endif
}

LOCAL BOOL CMDIsDone(U8 ucTag)
{
   return (g_tNVMEHostCmdTable[ucTag].ucCmdFinishFlag);
}

BOOL NVMe_IsFetchCmdEmpty()
{
    BOOL bEmpty = TRUE;
    U8 uCmdTag = 0;

    for (uCmdTag = 0; uCmdTag < MAX_SLOT_NUM; uCmdTag++)
    {
        if (!CMDIsDone(uCmdTag))
        {
            bEmpty = FALSE;
            break;
        }
    }
    return bEmpty;
}

void NVMe_ClearHostCmdTable(void)
{
    U8 uSlotIndex = 0;

    for (uSlotIndex = 0; uSlotIndex < MAX_SLOT_NUM; uSlotIndex++)
    {
        memset(&g_tNVMEHostCmdTable[uSlotIndex], 0, sizeof(NVME_CMD_TABLE));
    }
}

#if 0
BOOL NVME_IsHitHostLba(U32 ulStartLba, U32 ulSecCnt)
{
    U32 ulPendingLbaStart = 0;
    U32 ulPendingLbaEnd = 0;
    U8 uCmdTag = 0;
    BOOL bHit = FALSE;
    U32 ulNewLbaStart = ulStartLba;
    U32 ulNewLbaEnd = ulNewLbaStart + ulSecCnt - 1;

    for (uCmdTag = 0; uCmdTag < MAX_SLOT_NUM; uCmdTag++)
    {
        if (FALSE == CMDIsDone(uCmdTag))
        {
            bHit = TRUE;
            ulPendingLbaStart = NVME_GetStartLBA(uCmdTag);
            if(0 == ulPendingLbaStart && 0 == NVME_GetLbaLen(uCmdTag))
            {
                bHit = FALSE;
                continue;
            }
            ulPendingLbaEnd = ulPendingLbaStart + NVME_GetLbaLen(uCmdTag) - 1;
            if ((ulNewLbaStart > ulPendingLbaEnd) || (ulNewLbaEnd < ulPendingLbaStart))
            {
                bHit = FALSE;
            }

            if (TRUE == bHit)
            {
                break;
            }
        }
    }

    return bHit;
}
#endif

U32 NVME_GetStartLBA(U32 ulSlotNum)
{
    return g_tNVMEHostCmdTable[ulSlotNum].ulStartLba;
}

U32 NVME_GetLbaLen(U32 ulSlotNum)
{
    return g_tNVMEHostCmdTable[ulSlotNum].ulLbaLen;
}

U32 NVME_XferedSecs(U32 ulSlotNum)
{
    return g_tNVMEHostCmdTable[ulSlotNum].ulXferLba;
}

U32 NVME_XferedBytes(U32 ulSlotNum)
{
    return g_tNVMEHostCmdTable[ulSlotNum].ulXferByte;
}

/*U32 NVME_GetCmdCode(U32 ulSlotNum)
{
    return g_tNVMEHostCmdTable[ulSlotNum].ulCmdCode;
}*/

void NVME_XferOneSector(U32 ulSlotNum)
{
    g_tNVMEHostCmdTable[ulSlotNum].ulXferLba++;
}

void NVME_XferBtyes(U32 ulSlotNum,U32 ulXferBytes)
{
    g_tNVMEHostCmdTable[ulSlotNum].ulXferByte += ulXferBytes;
}
void NVMe_GetLbaFromHostCmdTable(LARGE_INTEGER *pHostAddr, LARGE_INTEGER *pLbaStartAddr,U8 CmdTag, U32 *pByteLen, U32 *pStartLba, U32 *pEndLba)
{
    U32 ulSecCnt;
    U32 ulBufferOffset;

    U32 ulHostStartAddr = (U32)g_tNVMEHostCmdTable[CmdTag].PRP1;
    U32 ulStartLbaAddrOffset = (ulHostStartAddr & HPAGE_SIZE_MSK)%SEC_SIZE;

    NVME_XferBtyes(CmdTag,(*pByteLen));

    if(pHostAddr->LowPart >= ulHostStartAddr)
    {
        if(0 == (pHostAddr->LowPart - ulHostStartAddr)%SEC_SIZE)
        {
            pLbaStartAddr->LowPart = pHostAddr->LowPart;
            ulBufferOffset = (pHostAddr->LowPart - ulHostStartAddr)/SEC_SIZE;
            *pStartLba = g_tNVMEHostCmdTable[CmdTag].ulStartLba + ulBufferOffset;
            ulSecCnt = (*pByteLen + SEC_SIZE - 1)/SEC_SIZE;
        }
        else if(0 == (pHostAddr->LowPart + ulStartLbaAddrOffset - ulHostStartAddr)%SEC_SIZE)
        {
            pLbaStartAddr->LowPart += ulStartLbaAddrOffset;
            ulBufferOffset = (pHostAddr->LowPart + ulStartLbaAddrOffset - ulHostStartAddr)/SEC_SIZE;
            *pStartLba = g_tNVMEHostCmdTable[CmdTag].ulStartLba + ulBufferOffset;
            ulSecCnt = (*pByteLen)/SEC_SIZE - 1;
        }
        else
        {
            DBG_Printf("HostAddr 0x%x Error!\n",pHostAddr->LowPart);
            DBG_Break();
        }
    }
    else
    {
        DBG_Printf("HostAddr 0x%x Error!\n",pHostAddr->LowPart);
        DBG_Break();
    }
    *pEndLba = *pStartLba + ulSecCnt;
    *pByteLen = 0;
    return;
}

BOOL NVME_HostIsLbaCmd(U32 ulSlotNum)
{
    U8 ucCmdCode = g_tNVMEHostCmdTable[ulSlotNum].ulCmdCode;
    U8 ucProtocol = g_tNVMEHostCmdTable[ulSlotNum].Protocal;
    if (((ucCmdCode == NVM_READ) || (ucCmdCode == NVM_WRITE)) && (ucProtocol != CMD_TYPE_ADMIN))
    {
        return TRUE;
    }

    return FALSE;
}

void NVME_HostCSetCmdFinishFlag(U8 ucTag)
{
    g_tNVMEHostCmdTable[ucTag].ucCmdFinishFlag = TRUE;
}

/*====================End of this file========================================*/

