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

Filename     :   AHCI_HostModel.c
Version      :   0.1
Date         :   2013.08.26
Author       :   bettywu

Description:  implement the function of AHCI host
Others:
Modification History:
20130826 create

******************************************************************************/
//include common
#include <time.h>

//include firmware
#include "AHCI_AhciSpec.h"

// include module
#include "SimATACmd.h"
#include "model_common.h"
#include "model_config.h"
#include "checklist_parse.h"
#include "base_pattern.h"
#include "AHCI_HostModel.h"
#include "AHCI_HostModelVar.h"
#include "AHCI_ControllerModel.h"
#include "AHCI_ControllerInterface.h"
#include "HostModel.h"
#include "HostInc.h"
#include "SimATA_Interface.h"
#include "SimATA_HSCMDCallBack.h"
#ifdef MIX_VECTOR
#include "MixVector_HostApp.h"
#endif

CRITICAL_SECTION g_CmdCriticalSection;
CRITICAL_SECTION g_DataTransferCriticalSection;

typedef struct _PORT_DSP
{

    CMD_HEADER *pCMDList;
    RFIS       *pRFis;
    U8          PostState;
}PORT_DSP, *PPORT_DSP;

CMD_TABLE g_tHostCmdTable[MAX_SLOT_NUM];
CMD_HEADER g_tCMDHeader[MAX_SLOT_NUM];
RFIS g_tPortRFis[MAX_SLOT_NUM];
#ifdef MIX_VECTOR
HOST_CMD g_pMvHCmdTable[MAX_SLOT_NUM];
#endif
UINT8 *g_pHostDataBuffer;
PORT_DSP g_tPortDsp;
volatile AHCI_PORT *g_pPortHBAReg = 0;
U8 g_aRandPrdSecCnt[RANDOM_SEC_ARRAY_SIZE];
U32 l_BackupSACT = 0;
LARGE_BUFFER_MANAGEMENT l_pLargeDataBuffer;
#define CMDIsIssued(uTag)   ((g_pPortHBAReg->CI >> uTag) & 0x1)
#define CMDIsOutStanding(uTag) ((g_pPortHBAReg->SACT >> uTag) & 0x1)
#define CMDIsIdle( uTag )   ( ( g_pPortHBAReg->CI & ( 1 << uTag ) ) == 0 )
#define CMDIsInActive( uTag ) ( ( g_pPortHBAReg->SACT & ( 1 << uTag ) ) == 0 )
// return 1 CMD is finished
#define CMDIsDone( uTag )     ( CMDIsIdle( uTag ) && CMDIsInActive( uTag ) )
#define CMDQIsEmpty    ( ( g_pPortHBAReg->CI | g_pPortHBAReg->SACT ) == 0 )

extern void SIM_HCTRegInit();

U32 g_WriteNotAlignCnt = 0;

void InitLargeDataBuffer()
{
    if (NULL != l_pLargeDataBuffer.pLargeDataBuffer)
    {
        free(l_pLargeDataBuffer.pLargeDataBuffer);
    }
    else
    {
        l_pLargeDataBuffer.pLargeDataBuffer = (U8*)malloc(AHCI_LARGE_BUFFER_SIZE);
        if (NULL == l_pLargeDataBuffer.pLargeDataBuffer)
        {
            DBG_Printf("InitLargeDataBuffer : malloc fail\n");
            DBG_Break();
        }
    }
};


void ResetLargeDataBuffer()
{
    l_pLargeDataBuffer.bUsed = 0;
    memset(l_pLargeDataBuffer.pLargeDataBuffer, 0, AHCI_LARGE_BUFFER_SIZE);
}

BOOL MallocLargeDataBuffer(U8 **pDataBuffer)
{
    BOOL bRtn = FALSE;
    if (l_pLargeDataBuffer.bUsed)
    {
        bRtn = FALSE;
    }
    else
    {
        *pDataBuffer = l_pLargeDataBuffer.pLargeDataBuffer;
        l_pLargeDataBuffer.bUsed = TRUE;
        bRtn = TRUE;
    }

    return bRtn;
}

BOOL ReleaseLargeDataBuffer(void)
{
    BOOL bRtn = FALSE;

    if (FALSE == l_pLargeDataBuffer.bUsed)
    {
        DBG_Printf("ReleaseLargeDataBuffer: you don't malloc any large data buffer \n");
        DBG_Break();
        bRtn = FALSE;
    }
    else
    {
        l_pLargeDataBuffer.bUsed = FALSE;
        bRtn = TRUE;
    }

    return bRtn;
}



//get CMD start lba
U32 AHCI_GetCMDStartLba(U8 CmdTag)
{
    return g_tHostCmdTable[CmdTag].ulStartLba;
}

__inline
void AHCI_ZeroMemory(U8* pBuffer, U32 BufferSize)
{
    ULONG i;

    for (i = 0; i < BufferSize; i++) {
        pBuffer[i] = 0;
    }
}

void AHCI_HostCFillCMDHeader(U8 CMDTag, U32 PrdTLen, U64 CTBaseAddr, BOOL bWrite)
{
    CMD_HEADER *pCmdHeader = (CMD_HEADER*)&g_tCMDHeader[CMDTag];

    pCmdHeader->WriteFlag = bWrite;

    pCmdHeader->CTBaseAddr = (U32)(CTBaseAddr & 0xFFFFFFFF);
    pCmdHeader->CTBaseAddrU = (U32)((CTBaseAddr >> 32) & 0xFFFFFFFF);

    pCmdHeader->PrdTLen = PrdTLen;

    pCmdHeader->CmdFisLen = MAX_CFIS_DW;

    return;
}

// if it is not normal r/w, ulStartLba set 0
void AHCI_HostResetCMDParam(U8 ucCmdTag, U32 ulStartLba, U32 ulSecLen)
{
    g_tHostCmdTable[ucCmdTag].ulStartLba = ulStartLba;
    g_tHostCmdTable[ucCmdTag].ulLbaLen = ulSecLen;
    g_tHostCmdTable[ucCmdTag].nTransferLba = 0;
    g_tHostCmdTable[ucCmdTag].nTransferByte = 0;

    memset(&g_tPortRFis[ucCmdTag], 0, sizeof(RFIS));
    AHCI_HostCClearCmdFinishFlag(ucCmdTag);

}

void AHCI_HostPortInit()
{
    LARGE_INTEGER ullBaseAddr = {0};

    // set Port x Command List Base Address
#ifdef MIX_VECTOR
    ullBaseAddr.QuadPart = (LONG64) g_pMvHCmdTable;
#else
    ullBaseAddr.QuadPart = (LONG64)(void *)&g_tCMDHeader;
#endif
#ifdef SIM
    g_pPortHBAReg = (AHCI_PORT *)&rP0_CLB;
#else
    g_pPortHBAReg = (AHCI_PORT *)GetVirtualAddrInLocalMem((U32)&rP0_CLB);
#endif

    g_pPortHBAReg->CLB = ullBaseAddr.LowPart;
    g_pPortHBAReg->CLBU = ullBaseAddr.HighPart;
    g_pPortHBAReg->CI = 0;
    g_pPortHBAReg->SACT = 0;

    ullBaseAddr.QuadPart = (LONG64)(void *)&g_tPortRFis;
    g_pPortHBAReg->FB = ullBaseAddr.LowPart;
    g_pPortHBAReg->FBU = ullBaseAddr.HighPart;

    g_tPortDsp.pCMDList = (CMD_HEADER *)&g_tCMDHeader;
    g_tPortDsp.pRFis = (RFIS *)&g_tPortRFis;
    g_tPortDsp.PostState = AHCI_PORT_WAIT_INI;
#ifndef MIX_VECTOR
    ParseRFisCallBack = AHCI_HostParseRFis;
#endif
}

void AHCI_HostResetPort()
{
    AHCI_COMMAND *pPortCmdReg = (AHCI_COMMAND *)&g_pPortHBAReg->CMD;

    l_BackupSACT = g_pPortHBAReg->SACT;


    pPortCmdReg->ST = 0;
}

void AHCI_ReSendCmd()
{
    U8 ucCmdTag = 0;
    U32 ulStartLba = 0;
    U16 usSecCnt = 0;

    EnterCriticalSection(&g_CmdCriticalSection);
    for(ucCmdTag = 0; ucCmdTag < MAX_SLOT_NUM; ucCmdTag++)
    {
        if(0 != (l_BackupSACT & (1<<ucCmdTag)) )
        {

            ulStartLba = g_tHostCmdTable[ucCmdTag].ulStartLba;
            usSecCnt = g_tHostCmdTable[ucCmdTag].ulLbaLen;
            AHCI_HostResetCMDParam(ucCmdTag, ulStartLba, usSecCnt);
            AHCI_HostCSetPxCI(ucCmdTag);
        }
    }

    LeaveCriticalSection(&g_CmdCriticalSection);
}

void AHCI_HostWaitPortStart()
{
    AHCI_COMMAND *pPortCmdReg = (AHCI_COMMAND *)&g_pPortHBAReg->CMD;
    switch (g_tPortDsp.PostState)
    {
    case AHCI_PORT_WAIT_INI:
        if (0x40006 == g_pPortHBAReg->CMD)
        {
            g_tPortDsp.PostState = AHCI_PORT_WAIT_FR;
            pPortCmdReg->FRE = 1;
        }

        break;
    case AHCI_PORT_WAIT_FR:
        if (1 == pPortCmdReg->FR)
        {
            g_tPortDsp.PostState = AHCI_PORT_WAIT_CR;
            pPortCmdReg->ST = 1;

        }
        break;
    case AHCI_PORT_WAIT_CR:
        if (1 == pPortCmdReg->CR)
        {
            g_tPortDsp.PostState = AHCI_PORT_START;
        }
        break;
    case AHCI_PORT_ERROR_WAIT_RESET:
        {
            AHCI_HostResetPort();
            g_tPortDsp.PostState = AHCI_PORT_ERROR_WAIT_CMDSTOP;
        }
        break;
    case AHCI_PORT_ERROR_WAIT_CMDSTOP:
        if (0 == pPortCmdReg->CR)
        {
            g_pPortHBAReg->SERR = 0;
            g_pPortHBAReg->IS = 0;
            pPortCmdReg->ST = 1;

            g_tPortDsp.PostState = AHCI_PORT_ERROR_RESTART;

            //g_tPortDsp.PostState = AHCI_PORT_START;
        }
        break;
    case AHCI_PORT_ERROR_RESTART:
    {
        if (1 == pPortCmdReg->CR)
        {
            if ((INVALID_8F == g_pPortHBAReg->SACT) &&
                (INVALID_8F == g_pPortHBAReg->CI))
            {
                g_pPortHBAReg->SACT = 0;
                g_pPortHBAReg->CI = 0;
            }
            else
            {
                DBG_Printf("AHCI_PORT_ERROR_RESTART, FW will Clear SACT and CI with 1 \n");
                DBG_Getch();
            }


            SIM_HCTRegInit();
            AHCI_ReSendCmd();
            g_tPortDsp.PostState = AHCI_PORT_START;
        }

    }
    case AHCI_PORT_START:
        break;
    }

   /* if ((AHCI_PORT_START == g_tPortDsp.PostState) &&
        (0 != l_BackupSACT))
    {
         AHCI_ReSendCmd();
    }*/
}

void AHCI_HostRandomizeSecCnt(void)
{
    int i;

    //srand((int)time(0));

    for (i = 0; i < RANDOM_SEC_ARRAY_SIZE; i++)
    {
        g_aRandPrdSecCnt[i] = (rand()%16 + 1) * 8;
    }
}

void Host_ModelInitInterface()
{
    AHCI_HostRandomizeSecCnt();
    AHCI_HostPortInit();
    g_pHostDataBuffer = (UINT8*)malloc(DATA_BUFFER_PER_CMD * MAX_SLOT_NUM + 1024);
    InitLargeDataBuffer();

    // PRD buffer need 512 align to transfer from OTFB
    if (0 != ((U64)g_pHostDataBuffer & 0x1FF) )
    {
        g_pHostDataBuffer = (U8*)((((U64)g_pHostDataBuffer >> 9) + 1) << 9);
    }
    InitializeCriticalSection(&g_CmdCriticalSection);
    //InitializeCriticalSection(&g_DataTransferCriticalSection);
    if (NULL == g_pHostDataBuffer)
    {
        DBG_Printf("AHCI_HostInit: malloc host data buffer failure!\n");
        DBG_Break();
    }

}

U32 AHCI_HostGetLbaLen(U8 uCmdTag)
{
    AHCI_H2D_REGISTER_FIS *pCmdFis = (AHCI_H2D_REGISTER_FIS*)&g_tHostCmdTable[uCmdTag].CmdFis;

    return g_tHostCmdTable[uCmdTag].ulLbaLen;
}

U8 AHCI_HostGetCmdCode(U8 uCmdTag)
{
    AHCI_H2D_REGISTER_FIS *pCmdFis = (AHCI_H2D_REGISTER_FIS*)&g_tHostCmdTable[uCmdTag].CmdFis;
    return pCmdFis->Command;
}

BOOL AHCI_HostLbaIsHit(U32 StartLba, U32 SectorCnt)
{
    U32 ulNewLbaStart = StartLba;
    U32 ulNewLbaEnd = 0;
    U32 ulPendingLbaStart = 0;
    U32 ulPendingLbaEnd = 0;
    U8 uCmdTag = 0;
    BOOL bHit = FALSE;

    ulNewLbaEnd = ulNewLbaStart + SectorCnt - 1;

    if (!CMDIsDone(0))
    {
        if ((g_tHostCmdTable[0].CmdFis.Command != ATA_CMD_READ_FPDMA_QUEUED)
            && (g_tHostCmdTable[0].CmdFis.Command != ATA_CMD_WRITE_FPDMA_QUEUED))
            return FALSE;
    }

    for (uCmdTag = 0; uCmdTag < 32; uCmdTag++)
    {
        if (!CMDIsDone(uCmdTag))
        {
            bHit = TRUE;
            ulPendingLbaStart = AHCI_GetCMDStartLba(uCmdTag);
            ulPendingLbaEnd = ulPendingLbaStart + AHCI_HostGetLbaLen(uCmdTag) - 1;
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



void AHCI_HostInputCFis(U8 ucCmdTag, AHCI_H2D_REGISTER_FIS *pCFisSrc)
{
    AHCI_H2D_REGISTER_FIS *pCFisDes = (AHCI_H2D_REGISTER_FIS*)&g_tHostCmdTable[ucCmdTag].CmdFis;

    memcpy(pCFisDes, pCFisSrc, 64);//copy 64 bytes

    return;
}

/*********************************************************************************************************
*Input: LARGE_INTEGER *pHostAddr
        U8 CmdTag
        U32 *pByteLen
*Output: LARGE_INTEGER *pHostAddr
         U32 *pByteLen
         U32 *pStartLba
         U32 *pEndLba
*Description:
**********************************************************************************************************/
BOOL AHCI_GetLbaFromAddrInPRD(LARGE_INTEGER *pHostAddr, U8 CmdTag, U32 *pByteLen, U32 *pStartLba, U32 *pEndLba)
{
    PRD_ENTRY *pPRDEntry = (PRD_ENTRY *)&g_tHostCmdTable[CmdTag].PrdTable;
    U32 ulPrdLen = g_tCMDHeader[CmdTag].PrdTLen;
    U32 ulPrdIndex;
    U32 ulCmdStartLba = AHCI_GetCMDStartLba(CmdTag);
    LARGE_INTEGER ullStartDBAInPrd = {0};
    LARGE_INTEGER ullEndDBAInPrd = {0};
    U32 ulCurPrdByteCnt;
    U32 ulPrdByteTotal = 0;
    U32 ulStartByteOffsetInLba;
    U32 ulEndByteOffsetInLba;
    U32 ulByteOffsetInPrd;
    U32 ulByteLen = *pByteLen;

    for (ulPrdIndex = 0; ulPrdIndex < ulPrdLen; ulPrdIndex++)
    {
        ulCurPrdByteCnt = pPRDEntry->DBC + 1;
        ullStartDBAInPrd.LowPart = pPRDEntry->DBA;
        ullStartDBAInPrd.HighPart = pPRDEntry->DBAU;
        ullEndDBAInPrd.QuadPart = ullStartDBAInPrd.QuadPart + ulCurPrdByteCnt;

        if ((pHostAddr->QuadPart >= ullStartDBAInPrd.QuadPart) &&
            (pHostAddr->QuadPart < ullEndDBAInPrd.QuadPart))
        {
            ulByteOffsetInPrd = (U32)(pHostAddr->QuadPart - ullStartDBAInPrd.QuadPart);
            ulStartByteOffsetInLba = (ulPrdByteTotal + ulByteOffsetInPrd) % SEC_SIZE;

            /*Get Start LBA*/
            if ( ulStartByteOffsetInLba < 8 )
            {
                *pStartLba = ulCmdStartLba + (ulPrdByteTotal + ulByteOffsetInPrd)/SEC_SIZE;
            }
            else
            {
                *pStartLba = ulCmdStartLba + (ulPrdByteTotal + ulByteOffsetInPrd)/SEC_SIZE + 1;
                pHostAddr->QuadPart += SEC_SIZE - ulStartByteOffsetInLba;
            }

            /*Get Transfer bytes this time*/
            if ( ulByteLen > (ulCurPrdByteCnt - ulByteOffsetInPrd) )
            {
                ulByteLen = ulCurPrdByteCnt - ulByteOffsetInPrd;
            }
            *pByteLen -= ulByteLen;

            /*Get End LBA*/
            ulEndByteOffsetInLba = (ulPrdByteTotal + ulByteOffsetInPrd + ulByteLen) % SEC_SIZE;
            if ( ulEndByteOffsetInLba >= 8 )
            {
                *pEndLba = ulCmdStartLba + (ulPrdByteTotal + ulByteOffsetInPrd + ulByteLen)/SEC_SIZE + 1;
            }
            else
            {
                *pEndLba = ulCmdStartLba + (ulPrdByteTotal + ulByteOffsetInPrd + ulByteLen)/SEC_SIZE;
            }

            break;
        }

        ulPrdByteTotal += ulCurPrdByteCnt;
        pPRDEntry++;
    } //for (ulPrdIndex = 0; ulPrdIndex < ulPrdLen; ulPrdIndex++)

    // not find host addr in PRDTable
    if (ulPrdIndex == ulPrdLen)
    {
        return FALSE;
    }

    return TRUE;
}

PRD_ENTRY* AHCI_GetAddrInPRD(LARGE_INTEGER HostAddr, U8 CmdTag, U32 *pByteLenInPrd, U32 *pStartLba)
{
    PRD_ENTRY *pPRDEntry = (PRD_ENTRY *)&g_tHostCmdTable[CmdTag].PrdTable;
    U8 *pDataBuffer = (U8*)(g_pHostDataBuffer + (CmdTag * DATA_BUFFER_PER_CMD));
    U32 ulPrdLen = g_tCMDHeader[CmdTag].PrdTLen;
    U32 ulPrdIndex = 0;
    U64 u64ByteCnt = 0;
    U32 ulByteCntPerPrd = 0;
    U32 ulStartLba = AHCI_GetCMDStartLba(CmdTag);
    LARGE_INTEGER ullStartDBAInPrd = {0};
    LARGE_INTEGER ullEndDBAInPrd = {0};
        //g_tHostCmdTable[CmdTag].CmdFis.


    for (ulPrdIndex = 0; ulPrdIndex < ulPrdLen; ulPrdIndex++)
    {
        ulByteCntPerPrd = pPRDEntry->DBC + 1;
        ullStartDBAInPrd.LowPart = pPRDEntry->DBA;
        ullStartDBAInPrd.HighPart = pPRDEntry->DBAU;
        ullEndDBAInPrd.QuadPart = ullStartDBAInPrd.QuadPart + ulByteCntPerPrd;

        if ((HostAddr.QuadPart >= ullStartDBAInPrd.QuadPart) &&
            (HostAddr.QuadPart < ullEndDBAInPrd.QuadPart))
        {
            *pStartLba = ulStartLba + (U32)((HostAddr.QuadPart - ullStartDBAInPrd.QuadPart) / 512);
            *pByteLenInPrd = (U32)(ullEndDBAInPrd.QuadPart - ullStartDBAInPrd.QuadPart);
            break;
        }

        ulStartLba += ulByteCntPerPrd / 512;
        pPRDEntry += 1;
    }

    // not find host addr in PRDTable
    if (ulPrdIndex == ulPrdLen)
    {
        pPRDEntry = NULL;
        pByteLenInPrd = 0;

    }

    return pPRDEntry;
}

void AHCI_CheckCmdData(U8 CmdTag)
{
    PRD_ENTRY *pPRDEntry = (PRD_ENTRY *)&g_tHostCmdTable[CmdTag].PrdTable;
    U32 ulPrdLen = g_tCMDHeader[CmdTag].PrdTLen;
    U32 ulPrdIndex = 0;
    U32 ulStartLba = AHCI_GetCMDStartLba(CmdTag);
    LARGE_INTEGER HostAddr;
    U32 *pStartLba;
    U32 ulRemainByteInPrd = 0;
    U32 ulCheckByteLen = 0;
    for (ulPrdIndex = 0; ulPrdIndex < ulPrdLen; ulPrdIndex++)
    {
        ulRemainByteInPrd = pPRDEntry->DBC + 1;

        HostAddr.LowPart = pPRDEntry->DBA;
        HostAddr.HighPart = pPRDEntry->DBAU;

        while(ulRemainByteInPrd > 0)
        {
            if (0 == (ulCheckByteLen & SEC_SIZE_MSK))
            {
                pStartLba = (U32*)HostAddr.QuadPart + 1;

                if (*pStartLba != ulStartLba)
                {
                    break;
                }
                ulStartLba++;
            }

            if (ulRemainByteInPrd >= SEC_SIZE)
            {
                ulRemainByteInPrd -= SEC_SIZE;
                HostAddr.QuadPart += SEC_SIZE;
            }
            else
            {
                HostAddr.QuadPart = ulRemainByteInPrd;
                ulRemainByteInPrd = 0;
            }
        }

        pPRDEntry += 1;
    }

    return;
}

void AHCI_HostFillDataToBuffer(PLARGE_INTEGER BufferAddr,U32 DstLba)
{
    U32 ulData = Host_UpdateDataCnt(DstLba);
    //LARGE_INTEGER = (LARGE_INTEGER)(*BufferAddr);

    BufferAddr->LowPart = ulData;
    BufferAddr->HighPart = DstLba;

}

void AHCI_HostPrepareRawData(U8 ucCmdTag, U8 ucPrdCnt, U8 *pCheckListDataBuf)
{
    PRD_ENTRY *pPRDEntry = (PRD_ENTRY *)&g_tHostCmdTable[ucCmdTag].PrdTable;
    U8 ucPrdIndex;
    U8 *pHostDataBuffer;
    U8 *pCurDataSrc = pCheckListDataBuf;
    U32 ulCurPrdLenByte;

    for (ucPrdIndex = 0; ucPrdIndex < ucPrdCnt; ucPrdIndex++)
    {
        pHostDataBuffer = (U8 *)pPRDEntry->DBA;
        ulCurPrdLenByte = pPRDEntry->DBC + 1;

        memcpy(pHostDataBuffer, pCurDataSrc, ulCurPrdLenByte);

        pCurDataSrc += ulCurPrdLenByte;
        pPRDEntry++;
    }

    return;
}

void AHCI_HostPrepareLbaData(U8 ucCmdTag, U8 ucPrdCnt, U32 ulStartLBA)
{
    PRD_ENTRY *pPRDEntry = (PRD_ENTRY *)&g_tHostCmdTable[ucCmdTag].PrdTable;
    U8 ucPrdIndex;
    LARGE_INTEGER ulDstHostAddr;
    U32 ulCurLBA = ulStartLBA;
    U32 ulCurPrdLenByte;
    U32 ulAddUpLenByte = 0;
    U32 ulSecOffsetByte;
    U32 ulSecRemainByte;
    U16 usPrdSecCnt;
    U16 usSecIndex;

    for (ucPrdIndex = 0; ucPrdIndex < ucPrdCnt; ucPrdIndex++)
    {
        ulDstHostAddr.LowPart = pPRDEntry->DBA;
        ulDstHostAddr.HighPart = pPRDEntry->DBAU;
        ulCurPrdLenByte = pPRDEntry->DBC + 1;

        ulSecOffsetByte = ulAddUpLenByte % SEC_SIZE;
        if (ulSecOffsetByte >= 8)
        {
            ulSecRemainByte = SEC_SIZE - ulSecOffsetByte;
            ulDstHostAddr.QuadPart += ulSecRemainByte;
            if (ulCurPrdLenByte > ulSecRemainByte)
            {
                usPrdSecCnt = (ulCurPrdLenByte - ulSecRemainByte) / SEC_SIZE;

                if ((ulCurPrdLenByte - ulSecRemainByte) % SEC_SIZE >= 8)
                {
                    usPrdSecCnt += 1;
                }
            }
            else
            {
                usPrdSecCnt = 0;
            }
        }
        else if (0 == ulSecOffsetByte)
        {
            usPrdSecCnt = ulCurPrdLenByte / SEC_SIZE;

            if ((ulCurPrdLenByte % SEC_SIZE) >= 8)
            {
                usPrdSecCnt += 1;
            }
        }
        else
        {
            DBG_Printf("AHCI_HostPrepareLbaData: PRD Length is not supported!\n");
            DBG_Break();
        }


        for (usSecIndex = 0; usSecIndex < usPrdSecCnt; usSecIndex++)
        {
            AHCI_HostFillDataToBuffer((PLARGE_INTEGER)ulDstHostAddr.QuadPart, ulCurLBA);
            ulDstHostAddr.QuadPart += SEC_SIZE;
            ulCurLBA++;
        }

        ulAddUpLenByte += ulCurPrdLenByte;
        pPRDEntry++;
    }

    return;
}

void AHCI_HostResetTrimData(TRIM_CMD_ENTRY *pTrimBlockEntry, U8 ucBlockCnt)
{
    U8 ucIndex;
    TRIM_CMD_ENTRY* pTrimCmdEntry;
    U32 ulLbaEntryIndex;
    U32 ulStartLba;
    U32 ulLba, ulLen;

    for (ucIndex = 0; ucIndex < ucBlockCnt; ucIndex++)
    {
        pTrimCmdEntry = &(pTrimBlockEntry[ucIndex]);

        for(ulLbaEntryIndex = 0; ulLbaEntryIndex < TRIM_LBA_RANGE_ENTRY_MAX; ulLbaEntryIndex++)
        {
            ulStartLba = pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaLow;
            ulLen = pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].RangeLength;
            for(ulLba=ulStartLba; ulLba< (ulStartLba + ulLen); ulLba++)
            {
                Host_SaveDataCnt(ulLba,0);//set lba's write count to 0
            }
         }
     }//while(FALSE == TRIM_ENTRY_EMPTY(g_TrimCmdEntryHead,g_TrimCmdEntryTail))

    return;
}


U8 AHCI_HostInput32KPRDT(PRD_ENTRY *pPRDEntry, U8 *pDataBuffer, U32 ulSecCnt)
{
    U32 ulDataBaseAddr = (U32)((UINT64)pDataBuffer & 0xFFFFFFFF);
    U32 ulDataBaseAddrUpper = (U32)(((UINT64)pDataBuffer >> 32) & 0xFFFFFFFF);
    U32 ulRemainSecCnt = ulSecCnt;
    U16 usCurPrdLenSec;
    U8 ucPrdCnt = 0;

    while (ulRemainSecCnt != 0)
    {
        pPRDEntry[ucPrdCnt].DBA = (U32)((ULONG_PTR)pDataBuffer & 0xFFFFFFFF);
        pPRDEntry[ucPrdCnt].DBAU = (U32)(((ULONG_PTR)pDataBuffer >> 32) & 0xFFFFFFFF);

        usCurPrdLenSec = (ulRemainSecCnt > SEC_PER_PRD) ? SEC_PER_PRD : ulRemainSecCnt;
        pPRDEntry[ucPrdCnt].DBC = (usCurPrdLenSec << SEC_SIZE_BITS) - 1;

        ulRemainSecCnt -= usCurPrdLenSec;
        pDataBuffer +=  (usCurPrdLenSec << SEC_SIZE_BITS);
        ucPrdCnt++;
    }

    return ucPrdCnt;
}

U8 AHCI_HostInput4KAlignPRDT(PRD_ENTRY *pPRDEntry, U8 *pDataBuffer, U32 ulSecCnt)
{
    U32 ulDataBaseAddr = (U32)((UINT64)pDataBuffer & 0xFFFFFFFF);
    U32 ulDataBaseAddrUpper = (U32)(((UINT64)pDataBuffer >> 32) & 0xFFFFFFFF);
    U32 ulRemainSecCnt = ulSecCnt;
    U16 usCurPrdLenSec;
    U8 ucPrdCnt = 0;

    while (ulRemainSecCnt != 0)
    {
        pPRDEntry[ucPrdCnt].DBA = (U32)((ULONG_PTR)pDataBuffer & 0xFFFFFFFF);
        pPRDEntry[ucPrdCnt].DBAU = (U32)(((ULONG_PTR)pDataBuffer >> 32) & 0xFFFFFFFF);

        usCurPrdLenSec = (rand()%16 + 1) * 8;//not more than 128 sectors
        if (usCurPrdLenSec > ulRemainSecCnt)
        {
            usCurPrdLenSec = ulRemainSecCnt;
        }
        else if ( (HOST_MAX_PRDT_NUM - 1) == ucPrdCnt)
        {
            if ((ulRemainSecCnt << SEC_SIZE_BITS) <= PRD_DATA_BYTE_MAX)
            {
                usCurPrdLenSec = ulRemainSecCnt;
            }
            else
            {
                DBG_Printf("AHCI_HostInputPRDT: data too long to put in PRD\n");
                DBG_Getch();
            }
        }

        pPRDEntry[ucPrdCnt].DBC = (usCurPrdLenSec << SEC_SIZE_BITS) - 1;

        ulRemainSecCnt -= usCurPrdLenSec;
        pDataBuffer +=  (usCurPrdLenSec << SEC_SIZE_BITS);
        ucPrdCnt++;
    }

    return ucPrdCnt;
}

U8 AHCI_HostInputRandomPRDT(PRD_ENTRY *pPRDEntry, U8 *pDataBuffer, U32 ulSecCnt, U8 ucCmdTag)
{
    U32 ulDataBaseAddr = (U32)((UINT64)pDataBuffer & 0xFFFFFFFF);
    U32 ulDataBaseAddrUpper = (U32)(((UINT64)pDataBuffer >> 32) & 0xFFFFFFFF);
    U32 ulRemainByteCnt = (ulSecCnt << SEC_SIZE_BITS);
    U32 ulCurPrdLenByte;
    U8 ucPrdCnt = 0;

    while (ulRemainByteCnt > 0)
    {
        /* generate gap between PRDs */
        if (((0 == ucPrdCnt) || (3 == ucPrdCnt)) && (0 == ucCmdTag % 5))
        {

            pDataBuffer += (rand()%8);

            //PRD butter align by 2 byte
            if (0 != ((UINT64)pDataBuffer & 0x1))
            {
                pDataBuffer += 1;
            }
        }

        pPRDEntry[ucPrdCnt].DBA = (U32)((UINT64)pDataBuffer & 0xFFFFFFFF);
        pPRDEntry[ucPrdCnt].DBAU = (U32)(((UINT64)pDataBuffer >> 32) & 0xFFFFFFFF);

        /* generate current prd length */
        if ((2 == ucCmdTag%4) && (0 == ucPrdCnt))
        {
            // Prd Len is 64's sector
            ulCurPrdLenByte = (SEC_PER_PRD << SEC_SIZE_BITS);
        }
        else if ( (1 == ucCmdTag%10) && (0 == ucPrdCnt) )
        {
            // Prd Len is 8 bytes.
            ulCurPrdLenByte = 8;
        }
        else if ((ucPrdCnt > (HOST_MAX_PRDT_NUM - 9)) &&(ulRemainByteCnt > PRD_DATA_BYTE_MAX))
        {
            ulCurPrdLenByte = PRD_DATA_BYTE_MAX;
        }
        //else if (3 == ucCmdTag) // less than 4k PRD Len Byte
        //{
        //
        //    ulCurPrdLenByte = ((rand()%8) << SEC_SIZE_BITS );
        //}
        else
        {
            //4K-align
            ulCurPrdLenByte = ( ((rand()%16 + 1) * 8) << SEC_SIZE_BITS );
        }


        if ( ulCurPrdLenByte > ulRemainByteCnt )
        {
            ulCurPrdLenByte = ulRemainByteCnt;
        }
        else if ( (HOST_MAX_PRDT_NUM - 1) == ucPrdCnt)
        {
            if (ulRemainByteCnt <= PRD_DATA_BYTE_MAX)
            {
                ulCurPrdLenByte = ulRemainByteCnt;
            }
            else
            {
                DBG_Printf("AHCI_HostInputPRDT: data too long to put in PRD\n");
                DBG_Getch();
            }
        }

        if (ucPrdCnt > HOST_MAX_PRDT_NUM)
        {
            DBG_Printf("ucPrdCnt = %d\n", ucPrdCnt);
            DBG_Break();
        }

        pPRDEntry[ucPrdCnt].DBC = ulCurPrdLenByte - 1;

        ulRemainByteCnt -= ulCurPrdLenByte;
        pDataBuffer +=  ulCurPrdLenByte;
        ucPrdCnt++;
    }//while (ulRemainByteCnt > 0)

    return ucPrdCnt;
}

U8 AHCI_HostInputPRDT(U8 ucCmdTag, U32 ulSecCnt)
{
    PRD_ENTRY *pPRDEntry = (PRD_ENTRY *)&g_tHostCmdTable[ucCmdTag].PrdTable;
    U8 *pDataBuffer = (U8*)g_tHostCmdTable[ucCmdTag].pDataBuffer;
    U8 ucPrdCnt;

    /* 32K PRD length */
    //ucPrdCnt = HCI_HostInput32KPRDT(pPRDEntry, pDataBuffer, ulSecCnt);

    /* 4K align random PRD length */
    //ucPrdCnt = AHCI_HostInput4KAlignPRDT(pPRDEntry, pDataBuffer, ulSecCnt);

    /* 8-byte align random PRD length */
    ucPrdCnt = AHCI_HostInputRandomPRDT(pPRDEntry, pDataBuffer, ulSecCnt, ucCmdTag);

    return ucPrdCnt;
}



void AHCI_HostFillLbaCmd(U8 ucCmdTag, AHCI_H2D_REGISTER_FIS *pCmdFis)
{
    U8 ucCmdCode = pCmdFis->Command;
    U32 ulStartLba = ATA_HostGetCmdLba(pCmdFis);
    U32 ulSecCnt = ATA_HostGetLbaCmdSecCnt(pCmdFis);
    U8 ucPrdCnt = 0;
    BOOL bWrite = FALSE;
    UINT64 *pCmdTableAddr = (UINT64*)&g_tHostCmdTable[ucCmdTag];

    EnterCriticalSection(&g_CmdCriticalSection);

    AHCI_HostInputCFis(ucCmdTag, pCmdFis);

    ucPrdCnt = AHCI_HostInputPRDT(ucCmdTag, ulSecCnt);

    if ((ucCmdCode == ATA_CMD_WRITE_FPDMA_QUEUED)
        || (ucCmdCode == ATA_CMD_WRITE_DMA) || (ucCmdCode == ATA_CMD_WRITE_DMA_EXT)
        || (ucCmdCode == ATA_CMD_WRITE_SECTOR) || (ucCmdCode == ATA_CMD_WRITE_SECTOR_EXT)
        || (ucCmdCode == ATA_CMD_WRITE_MULTIPLE) || (ucCmdCode == ATA_CMD_WRITE_MULTIPLE_EXT))
    {
        bWrite = TRUE;

        AHCI_HostPrepareLbaData(ucCmdTag, ucPrdCnt, ulStartLba);
    }

    AHCI_HostCFillCMDHeader(ucCmdTag, ucPrdCnt, (UINT64)pCmdTableAddr, bWrite);

    AHCI_HostResetCMDParam(ucCmdTag, ulStartLba, ulSecCnt);


    if ((ucCmdCode == ATA_CMD_WRITE_FPDMA_QUEUED) || (ucCmdCode == ATA_CMD_READ_FPDMA_QUEUED))
    {
        AHCI_HostCSetPxSACT(ucCmdTag);
    }

    AHCI_HostCSetPxCI(ucCmdTag);

    LeaveCriticalSection(&g_CmdCriticalSection);

    return;
}

void AHCI_HostFillHSCmd(U8 ucCmdTag, HSCMD_INFO *pHSCmd)
{
    AHCI_H2D_REGISTER_FIS *pCFis = (AHCI_H2D_REGISTER_FIS*)&pHSCmd->RowCmd;
    U16 usSecCnt = pHSCmd->DataSecCnt;
    U8 ucProtocol = pHSCmd->Protocal;
    U8 ucPrdCnt = 0;
    BOOL bWrite = FALSE;
    UINT64 *pCmdTableAddr = 0;

    EnterCriticalSection(&g_CmdCriticalSection);

    AHCI_HostInputCFis(ucCmdTag, pCFis);

    if (usSecCnt != 0)
    {
        ucPrdCnt = AHCI_HostInputPRDT(ucCmdTag, (U32)usSecCnt);

        if ((PROT_PIO_OUT == ucProtocol) || (PROT_DMA_OUT == ucProtocol))
        {
            bWrite = TRUE;

            AHCI_HostPrepareRawData(ucCmdTag, ucPrdCnt, pHSCmd->pDataBuffer);
        }
    }

    /* for TRIM commnd, reset DataCnt */
    if (ATA_CMD_DATA_SET_MANAGEMENT == pCFis->Command)
    {
        if (TRUE == (pCFis->Feature7_0 & 1))
        {
            AHCI_HostResetTrimData((TRIM_CMD_ENTRY *)(pHSCmd->pDataBuffer), pCFis->Count7_0);
        }
    }

    pCmdTableAddr = (UINT64*)&g_tHostCmdTable[ucCmdTag];

    AHCI_HostCFillCMDHeader(ucCmdTag, ucPrdCnt, (UINT64)pCmdTableAddr, bWrite);

    AHCI_HostResetCMDParam(ucCmdTag, 0, usSecCnt);

    AHCI_HostCSetPxCI(ucCmdTag);

    LeaveCriticalSection(&g_CmdCriticalSection);

    return;
}

void AHCI_HostCopyDataBack(U8 ucCmdTag, U8 *pCheckListDataBuf)
{
    PRD_ENTRY *pPRDEntry = (PRD_ENTRY *)&g_tHostCmdTable[ucCmdTag].PrdTable;
    U16 usPrdCnt = g_tCMDHeader[ucCmdTag].PrdTLen;
    U16 usPrdIndex;
    U8 *pHostDataBuffer;
    U8 *pCurDataDes = pCheckListDataBuf;
    U32 ulCurPrdLenByte;

    for (usPrdIndex = 0; usPrdIndex < usPrdCnt; usPrdIndex++)
    {
        pHostDataBuffer = (U8 *)pPRDEntry->DBA;
        ulCurPrdLenByte = pPRDEntry->DBC + 1;

        memcpy(pCurDataDes, pHostDataBuffer, ulCurPrdLenByte);

        pCurDataDes += ulCurPrdLenByte;
        pPRDEntry++;
    }

    return ;
}

U8 *AHCI_HostGetCmdCommonBuffer(U8 uCmdTag)
{
    return (U8*)(g_pHostDataBuffer + (uCmdTag * DATA_BUFFER_PER_CMD));
}

void ResetDataBuffer(U8 uCmdTag)
{
    if (g_tHostCmdTable[uCmdTag].pDataBuffer != AHCI_HostGetCmdCommonBuffer(uCmdTag))
    {
        g_tHostCmdTable[uCmdTag].pDataBuffer = AHCI_HostGetCmdCommonBuffer(uCmdTag);
        ReleaseLargeDataBuffer();
    }

}

void AHCI_HostCmdFinishCallBack(U8 ucCmdTag)
{
    ResetDataBuffer(ucCmdTag);
}



BOOL AHCI_HostSendNCQCmd(HSCMD_INFO *pHSCmd)
{
    AHCI_H2D_REGISTER_FIS *pCFis = (AHCI_H2D_REGISTER_FIS*)&pHSCmd->RowCmd;
    U8 ucCmdTag;
    U32 ulSecCnt = ATA_HostGetNCQSecCnt(pCFis);
    U32 ulStartLba = ATA_HostGetCmdLba(pCFis);
    BOOL bRtn = FALSE;

    /* avoid LBA overlap with processing commands */
    if (AHCI_HostLbaIsHit(ulStartLba, ulSecCnt))
    {
        return FALSE;
    }

    /* Fill command */
    for (ucCmdTag = 0; ucCmdTag < 32; ucCmdTag++)
    {
        if (CMDIsDone(ucCmdTag))
        {
            U32 ulSecCnt = ATA_HostGetLbaCmdSecCnt(pCFis);
            if (ulSecCnt >MAX_CMD_SEC_CNT)
            {
                if (FALSE == MallocLargeDataBuffer(&g_tHostCmdTable[ucCmdTag].pDataBuffer))
                {
                    bRtn = FALSE;
                    break;
                }
            }

            AHCI_HostFillLbaCmd(ucCmdTag, pCFis);
            bRtn = TRUE;
            break;
        }
    }

    return bRtn;
}

BOOL AHCI_HostSendDMACmd(HSCMD_INFO *pHSCmd)
{
    U8 ucCmdTag;
    AHCI_H2D_REGISTER_FIS *pCFis = (AHCI_H2D_REGISTER_FIS*)&pHSCmd->RowCmd;

    /* command queue must be empty */
    if (!CMDQIsEmpty)
    {
        return FALSE;
    }

    /* Fill command */
    ucCmdTag = rand() % MAX_SLOT_NUM;

    MallocLargeDataBuffer(&g_tHostCmdTable[ucCmdTag].pDataBuffer);
    AHCI_HostFillLbaCmd(ucCmdTag, pCFis);

    return TRUE;
}

void AHCI_HostParseRFis(U8 ucCmdTag,U32* pFeedBack)
{
    RD2HFIS* pD2HFis = (RD2HFIS*)&g_tPortRFis[ucCmdTag].RD2HFis;

    *pFeedBack = (BYTE_0(pD2HFis->Count) << 24) | pD2HFis->LBALo;
    *(pFeedBack + 1) = (BYTE_1(pD2HFis->Count) << 24) | pD2HFis->LBAHi;
}

BOOL AHCI_HostSendHSCMD(HSCMD_INFO *pHSCmd)
{
    U8 ucCmdTag;
    U8 ucProtocol = pHSCmd->Protocal;

    /* command queue must be empty */
    if (!CMDQIsEmpty)
    {
        return FALSE;
    }

    /* Fill command */
    ucCmdTag = rand() % MAX_SLOT_NUM;

    MallocLargeDataBuffer(&g_tHostCmdTable[ucCmdTag].pDataBuffer);
    AHCI_HostFillHSCmd(ucCmdTag, pHSCmd);

    while(!CMDQIsEmpty)
    {
        /* wait HSCMD has been done,then return to callback*/
       Sleep(10);
    }

    if ((PROT_PIO_IN== ucProtocol) || (PROT_DMA_IN == ucProtocol))
    {
        if (1 == g_tPortRFis[ucCmdTag].RD2HFis.Error)
        {
            pHSCmd->bDataValid = TRUE;
        }
        AHCI_HostCopyDataBack(ucCmdTag, pHSCmd->pDataBuffer);
    }

    return TRUE;
}

void AHCI_HostCmdDataFinishPro(U8 nTag)
{
    AHCI_HostCmdFinishCallBack(nTag);
    AHCI_HostCSetCmdFinishFlag(nTag);
    return;
}

void Host_WriteToDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag)
{
    LARGE_INTEGER ullHostAddr = {0};
    U32 ulByteLenInPrd = 0;
    U32 ulLeaveByteLen = ByteLen;
    LARGE_INTEGER ullStartLba = {0};
    U32 ulStartLba = 0;
    U32 ulLbaIndex = 0;
    U32 ulCmdLbaLen;
    U32 ulCmdByteLen;
    U32 ulEndLba = 0;


#ifdef MIX_VECTOR
    ulCmdLbaLen = MV_GetHostCmdDiskBLbaLength(nTag);
    ulCmdLbaLen += MV_GetHostCmdDiskCLbaLength(nTag);
#else
    ulCmdLbaLen = AHCI_HostGetLbaLen(nTag);
#endif

    ulCmdByteLen = (ulCmdLbaLen << SEC_SIZE_BITS);

    ullHostAddr.HighPart = HostAddrHigh;
    ullHostAddr.LowPart = HostAddrLow;

    AHCI_WriteToDevice(BufferAddr, ByteLen, (U8*)ullHostAddr.QuadPart);

    EnterCriticalSection(&g_CmdCriticalSection);

    if ( FALSE == AHCI_GetLbaFromAddrInPRD(&ullHostAddr, nTag, &ulLeaveByteLen, &ulStartLba, &ulEndLba) )
    {
        DBG_Printf("AHCI_HostReadFromDevice: can't find host address in PRD Table Tag = 0x%x!, HostAddr = 0x%x\n", nTag, ullHostAddr);
        DBG_Break();
    }

    g_tHostCmdTable[nTag].nTransferByte += ByteLen;

    //check if finish data transfering on current tag
    if (g_tHostCmdTable[nTag].nTransferByte == ulCmdByteLen)
    {
        AHCI_HostCmdDataFinishPro(nTag);
    }
    else if (g_tHostCmdTable[nTag].nTransferByte > ulCmdByteLen)
    {
        DBG_Printf("Host_WriteToDeviceInterface Tag 0x%x ulCmdByteLen 0x%x\n", nTag, ulCmdByteLen);
        DBG_Break();
    }

    LeaveCriticalSection(&g_CmdCriticalSection);
}

extern void Host_ReadDataFromBuffer(PLARGE_INTEGER BufferAddr, U32 DstLba);
void Host_ReadFromDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag)
{
    LARGE_INTEGER ullHostAddr = {0};
    //PRD_ENTRY* pPrdEntryAddr;
    LARGE_INTEGER ullStartLba = {0};
    U32 ulByteLenInPrd = 0;
    U32 ulLeaveByteLen = ByteLen;

    U32 ulStartLba = 0;
    U32 ulEndLba = 0;
    U32 ulLbaIndex = 0;
    U32 ulCmdLbaLen;
    U32 ulCmdByteLen;

    U8 ucCmdCode = g_tHostCmdTable[nTag].CmdFis.Command;
    BOOL bLbaCmd = ATA_HostIsLbaCmd(ucCmdCode);

#ifdef MIX_VECTOR
    ulCmdLbaLen = MV_GetHostCmdDiskBLbaLength(nTag);
    ulCmdLbaLen += MV_GetHostCmdDiskCLbaLength(nTag);
#else
    ulCmdLbaLen = AHCI_HostGetLbaLen(nTag);
#endif

    ulCmdByteLen = (ulCmdLbaLen << SEC_SIZE_BITS);

    ullHostAddr.HighPart = HostAddrHigh;
    ullHostAddr.LowPart = HostAddrLow;

    AHCI_ReadFromDevice(BufferAddr, ByteLen, (U8*)ullHostAddr.QuadPart);

    EnterCriticalSection(&g_CmdCriticalSection);

    // only check data for every LBA in AHCI mode
    //In MixVect EVN, CMD is not LBACmd for ever,
    //so this is if section will not be runed in MixVect
    if (TRUE == bLbaCmd)
    {
        while (0 != ulLeaveByteLen)
        {
            U32 ulEndLba = 0;

            //get lba by Host address
            if (FALSE == AHCI_GetLbaFromAddrInPRD(&ullHostAddr, nTag, &ulLeaveByteLen, &ulStartLba, &ulEndLba))
            {
                DBG_Printf("AHCI_HostReadFromDevice: can't find host address in PRD Table Tag = 0x%x!, HostAddr = 0x%x\n", nTag, ullHostAddr);
                DBG_Break();
            }

            for (ulLbaIndex = ulStartLba; ulLbaIndex < ulEndLba; ulLbaIndex++) //read one sector every time
            {
                //do data check
                Host_ReadDataFromBuffer((PLARGE_INTEGER)ullHostAddr.QuadPart, ulLbaIndex);

                ullHostAddr.QuadPart += SEC_SIZE;
                g_tHostCmdTable[nTag].nTransferLba += 1;
            }
        }
    }


    g_tHostCmdTable[nTag].nTransferByte += ByteLen;

    //chekc if finish data read on current tag
    if (g_tHostCmdTable[nTag].nTransferByte == ulCmdByteLen)
    {
        if ((TRUE == bLbaCmd)
            && (g_tHostCmdTable[nTag].nTransferLba != ulCmdLbaLen))
        {
            DBG_Printf("Host_ReadFromDeviceInterface bLbaCmd Tag 0x%x ulCmdByteLen 0x%x\n", nTag, ulCmdByteLen);
            DBG_Break();
        }

        AHCI_HostCmdDataFinishPro(nTag);
    }
    else if (g_tHostCmdTable[nTag].nTransferByte > ulCmdByteLen)
    {
        DBG_Printf("Host_ReadFromDeviceInterface Tag 0x%x ulCmdByteLen 0x%x\n", nTag, ulCmdByteLen);
        DBG_Break();
    }

    LeaveCriticalSection(&g_CmdCriticalSection);
    return;
}

// begin:interface for SGE
void AHCI_HostWriteToOTFB(U32 HostAddrHigh, U32 HostAddrLow, U32 OTFBAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = OTFB_START_ADDRESS + OTFBAddr;

    Host_WriteToDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}

void AHCI_HostWriteToDram(U32 HostAddrHigh, U32 HostAddrLow, U32 DramAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = DRAM_START_ADDRESS + DramAddr;

    Host_WriteToDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}


void AHCI_HostReadFromOTFB(U32 HostAddrHigh, U32 HostAddrLow, U32 OTFBAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = OTFB_START_ADDRESS + OTFBAddr;

    Host_ReadFromDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}

void AHCI_HostReadFromDram(U32 HostAddrHigh, U32 HostAddrLow, U32 DramAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = DRAM_START_ADDRESS + DramAddr;

    Host_ReadFromDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}

// end:interface for SGE

BOOL Host_IsCMDEmptyInterface()
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
/*
Initialize AHCI HBA.
Once the function returns, it makes sure that PortState == AHCI_PORT_START.
*/
BOOL AHCI_InitHBA()
{
    while (AHCI_PORT_START != g_tPortDsp.PostState)
    {
        AHCI_HostWaitPortStart();
    }
    return TRUE;
}

void AHCI_HCmdToRowCmd(HCMD_INFO* pHcmd)
{
    AHCI_H2D_REGISTER_FIS *pCmdFis = (AHCI_H2D_REGISTER_FIS*)&pHcmd->HSCmd.RowCmd;
    HSCMD_INFO *pHSCmd = &pHcmd->HSCmd;
    U32 StartLba = pHcmd->StartLba;
    U32 SecCnt = pHcmd->SecCnt;
    U8 ucSubCmd;

    switch(pHcmd->CmdType)
    {
    case CMD_TYPE_READ:
    case CMD_TYPE_WRITE:
        break;
    case CMD_TYPE_TRIM:
        pCmdFis->Command = ATA_CMD_DATA_SET_MANAGEMENT;
        strcpy(pHSCmd->CmdDsc, "Trim");
        pHSCmd->DataSecCnt = SecCnt;
        pHSCmd->Protocal = PROT_DMA_OUT;
        pHSCmd->pDataBuffer = (U8 *)g_TrimCmdEntry;
        break;
    case CMD_TYPE_STANDBY:
        pCmdFis->Command = ATA_CMD_STANDBY_IMMEDIATE;
        strcpy(pHSCmd->CmdDsc, "Standby");
        pHSCmd->DataSecCnt = 0;
        pHSCmd->Protocal = PROT_NONDATA;
        break;
    case CMD_TYPE_HSCMD:
        SecCnt = pHSCmd->DataSecCnt;
        break;
    default:
        DBG_Printf("HCmd CmdType error\n");
        DBG_Break();
        break;
    }

    if(CMD_TYPE_TRIM == pHcmd->CmdType)
    {
        pCmdFis->LBA7_0 = StartLba & 0xFF;
        pCmdFis->LBA15_8 = (StartLba >> 8) & 0xFF;
        pCmdFis->LBA23_16 = (StartLba >> 16) & 0xFF;
        pCmdFis->LBA31_24 = (StartLba >>24) & 0xFF;
    }

    if (ATA_CMD_VENDER_DEFINE == pCmdFis->Command)
    {
        ucSubCmd = pCmdFis->Feature7_0;
        if (VIA_TRACE_GETLOGDATA == ucSubCmd)
        {
            pHSCmd->DataSecCnt = pHcmd->SecCnt;
            SecCnt = pHcmd->SecCnt;

            pCmdFis->LBA7_0 = StartLba & 0xFF;
            pCmdFis->LBA15_8 = (StartLba >> 8) & 0xFF;
            pCmdFis->LBA23_16 = (StartLba >> 16) & 0xFF;
            pCmdFis->LBA31_24 = (StartLba >>24) & 0xFF;
            pCmdFis->Feature15_8 = pHcmd->McuId;
        }
        else if (VIA_SUBCMD_MEM_READ == ucSubCmd || VIA_SUBCMD_MEM_WRITE == ucSubCmd)
        {
            //DW0:DevAddr;
            pCmdFis->LBA7_0 = pHcmd->StartLba & 0xFF;
            pCmdFis->LBA15_8 = (pHcmd->StartLba >> 8) & 0xFF;
            pCmdFis->LBA23_16 = (pHcmd->StartLba >> 16) & 0xFF;
            pCmdFis->Count7_0 = (pHcmd->StartLba >> 24) & 0xFF;
            //DW1:DataLen
            pCmdFis->LBA31_24 = (U8)(pHcmd->SecCnt & 0x000000ff);
            pCmdFis->LBA39_32 = (U8)((pHcmd->SecCnt >> 8)&0x000000ff);
            pHSCmd->DataSecCnt = pHcmd->SecCnt/SEC_SIZE;
            pCmdFis->Feature15_8 = pHcmd->McuId;
        }
        else if (VIA_SUBCMD_FLASH_READ == ucSubCmd || VIA_SUBCMD_FLASH_WRITE == ucSubCmd || VIA_SUBCMD_FLASH_ERASE == ucSubCmd)
        {
            //DW0:PuMsk;
            pCmdFis->LBA7_0 = pHcmd->StartLba & 0xFF;
            pCmdFis->LBA15_8 = (pHcmd->StartLba >> 8) & 0xFF;
            pCmdFis->LBA23_16 = (pHcmd->StartLba >> 16) & 0xFF;
            pCmdFis->Count7_0 = (pHcmd->StartLba >> 24) & 0xFF;
            //DW1:Blk & Page
            pCmdFis->LBA31_24 = (U8)(pHcmd->SecCnt & 0x000000ff);
            pCmdFis->LBA39_32 = (U8)((pHcmd->SecCnt >> 8)&0x000000ff);
            pCmdFis->LBA47_40 = (U8)((pHcmd->SecCnt >> 16)&0x000000ff);
            pCmdFis->Feature15_8 = pHcmd->McuId;
        }
        else if(VIA_SUBCMD_TRACELOG_CONTROL == ucSubCmd)
        {
            //SecCnt
            pCmdFis->LBA23_16 = (pHcmd->SecCnt ) & 0xFF;
            pCmdFis->Count7_0 = (pHcmd->SecCnt >> 8) & 0xFF;
            //McuId
            pCmdFis->Feature15_8 = pHcmd->McuId;
        }
    }
    else if (CMD_TYPE_READ != pHcmd->CmdType &&
            CMD_TYPE_WRITE != pHcmd->CmdType)
    {
        pCmdFis->Count7_0 = (UCHAR)(SecCnt & 0x000000ff);
        pCmdFis->Count15_8 = (UCHAR)((SecCnt>>8)&0x000000ff);
    }

    if (ATA_CMD_DATA_SET_MANAGEMENT == pCmdFis->Command)
    {
        pCmdFis->Feature7_0 |= 0x1;
    }

    pCmdFis->C = TRUE;
    pCmdFis->FisType = RH2D_FIS_TYPE;
}

//BOOL Host_SendOneCmdInterface(HSCMD_INFO *pHSCmd)
BOOL Host_SendOneCmdInterface(HCMD_INFO* pHcmd)
{
    AHCI_H2D_REGISTER_FIS *pCFis = (AHCI_H2D_REGISTER_FIS*)(&pHcmd->HSCmd.RowCmd);
    BOOL bRtn = FALSE;
    U8 ucCmdCode = 0;

    AHCI_HCmdToRowCmd(pHcmd);
    ucCmdCode = pCFis->Command;

    if (AHCI_PORT_START != g_tPortDsp.PostState)
    {
        AHCI_HostWaitPortStart();
        return FALSE;
    }

    if ((ucCmdCode == ATA_CMD_READ_FPDMA_QUEUED) || (ucCmdCode == ATA_CMD_WRITE_FPDMA_QUEUED))
    {
        bRtn = AHCI_HostSendNCQCmd(&pHcmd->HSCmd);
    }
    else if (TRUE == ATA_HostIsLbaCmd(ucCmdCode))
    {
        bRtn = AHCI_HostSendDMACmd(&pHcmd->HSCmd);
    }
    else
    {
        bRtn = AHCI_HostSendHSCMD(&pHcmd->HSCmd);
    }

    return bRtn;
}

void Host_SimClearHCmdQueueInterface(void)
{
    U8 uSlotIndex = 0;

    for (uSlotIndex = 0; uSlotIndex < MAX_SLOT_NUM; uSlotIndex++)
    {
        memset(&g_tHostCmdTable[uSlotIndex], 0, sizeof(CMD_TABLE));
        g_tHostCmdTable[uSlotIndex].pDataBuffer = (U8*)(g_pHostDataBuffer + (uSlotIndex * DATA_BUFFER_PER_CMD));
    }

    ResetLargeDataBuffer();
}
BOOL Host_IsHitHCmdTable(U32 ulSystemLba)
{
    U8 uSlotIndex = 0;
    U32 ulStartLba,ulEndLba;

    for (uSlotIndex = 0; uSlotIndex < MAX_SLOT_NUM; uSlotIndex++)
    {
        ulStartLba = g_tHostCmdTable[uSlotIndex].ulStartLba;
        ulEndLba = ulStartLba + g_tHostCmdTable[uSlotIndex].ulLbaLen - 1;
        if(ulSystemLba >= ulStartLba && ulSystemLba <= ulEndLba)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL AHCI_IsHBARegActive(void)
{
    if ((0 != g_pPortHBAReg->SACT) || (0 != g_pPortHBAReg->CI))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



BOOL AHCI_NCQCmdErrorInterrupt()
{
    g_tPortDsp.PostState = AHCI_PORT_ERROR_WAIT_RESET;

    return TRUE;
}

