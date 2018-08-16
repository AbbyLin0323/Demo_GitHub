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
*******************************************************************************/

#include "model_common.h"
#include "BaseDef.h"
#include "SimATACmd.h"
#include "SimATA_Interface.h"



U32 ATA_HostGetCmdLba(PSATA_H2D_REGISTER_FIS pCFis)
{
    U32 ulStartLba = pCFis->LBA7_0 + (pCFis->LBA15_8 << 8)
                         + (pCFis->LBA23_16 << 16) + (pCFis->LBA31_24 << 24);

    return ulStartLba;
}

U32 ATA_HostGetNCQSecCnt(PSATA_H2D_REGISTER_FIS pCFis)
{
    U32 ulSecCnt;

    ulSecCnt = pCFis->Feature7_0 + (pCFis->Feature15_8 << 8);
    if (0 == ulSecCnt)
    {
        ulSecCnt = (1 << 16);
    }

    return ulSecCnt;
}

U32 ATA_HostGetLBA48SecCnt(PSATA_H2D_REGISTER_FIS pCFis)
{
    U32 ulSecCnt;

    ulSecCnt = pCFis->Count7_0 + (pCFis->Count15_8 << 8);
    if (0 == ulSecCnt)
    {
        ulSecCnt = (1 << 16);
    }

    return ulSecCnt;
}

U32 ATA_HostGetLBA28SecCnt(PSATA_H2D_REGISTER_FIS pCFis)
{
    U32 ulSecCnt;

    ulSecCnt = pCFis->Count7_0;
    if (0 == ulSecCnt)
    {
        ulSecCnt = (1 << 8);
    }

    return ulSecCnt;
}

U32 ATA_HostGetLbaCmdSecCnt(PSATA_H2D_REGISTER_FIS pCFis)
{
    U8 ucCmdCode = pCFis->Command;
    U32 ulSecCnt;

    switch (ucCmdCode)
    {
    case ATA_CMD_WRITE_FPDMA_QUEUED:
    case ATA_CMD_READ_FPDMA_QUEUED:
        ulSecCnt = ATA_HostGetNCQSecCnt(pCFis);
        break;

    case ATA_CMD_READ_DMA:
    case ATA_CMD_WRITE_DMA:
    case ATA_CMD_READ_SECTOR:
    case ATA_CMD_WRITE_SECTOR:
    case ATA_CMD_READ_MULTIPLE:
    case ATA_CMD_WRITE_MULTIPLE:
        ulSecCnt = ATA_HostGetLBA48SecCnt(pCFis);
        break;

    case ATA_CMD_READ_DMA_EXT:
    case ATA_CMD_WRITE_DMA_EXT:
    case ATA_CMD_READ_SECTOR_EXT:
    case ATA_CMD_READ_MULTIPLE_EXT:
    case ATA_CMD_WRITE_SECTOR_EXT:
    case ATA_CMD_WRITE_MULTIPLE_EXT:
        ulSecCnt = ATA_HostGetLBA28SecCnt(pCFis);
        break;

    default:
        DBG_Printf("ATA_HostGetLbaCmdSecCnt: command code 0x%x isn't a LBA command\n", ucCmdCode);
        DBG_Break();
        break;
    }

    return ulSecCnt;
}

BOOL ATA_IsNCQCMD(U8 ucCmdCode)
{
    BOOL bNCQCmd = FALSE;

    if(  (ATA_CMD_READ_FPDMA_QUEUED     == ucCmdCode)
        ||(ATA_CMD_WRITE_FPDMA_QUEUED   == ucCmdCode)
        ||(ATA_CMD_NCQ_NON_DATA         == ucCmdCode)
        ||(ATA_CMD_SEND_FPDMA_QUEUED    == ucCmdCode)
        ||(ATA_CMD_RECEIVE_FPDMA_QUEUED == ucCmdCode))
    {
        bNCQCmd = TRUE;
    };

    return bNCQCmd;
}

BOOL ATA_HostIsLbaCmd(U8 ucCmdCode)
{
    BOOL bHostLbaCmd = FALSE;

    if ((ucCmdCode == ATA_CMD_WRITE_FPDMA_QUEUED) || (ucCmdCode == ATA_CMD_READ_FPDMA_QUEUED)
        || (ucCmdCode == ATA_CMD_READ_DMA) || (ucCmdCode == ATA_CMD_WRITE_DMA)
        || (ucCmdCode == ATA_CMD_READ_DMA_EXT) || (ucCmdCode == ATA_CMD_WRITE_DMA_EXT)
        || (ucCmdCode == ATA_CMD_READ_SECTOR) || (ucCmdCode == ATA_CMD_WRITE_SECTOR)
        || (ucCmdCode == ATA_CMD_READ_SECTOR_EXT) || (ucCmdCode == ATA_CMD_WRITE_SECTOR_EXT)
        || (ucCmdCode == ATA_CMD_READ_MULTIPLE) || (ucCmdCode == ATA_CMD_WRITE_MULTIPLE)
        || (ucCmdCode == ATA_CMD_READ_MULTIPLE_EXT) || (ucCmdCode == ATA_CMD_WRITE_MULTIPLE_EXT)
        || (ucCmdCode == ATA_CMD_NCQ_NON_DATA) || (ucCmdCode == ATA_CMD_SEND_FPDMA_QUEUED)
        || (ucCmdCode == ATA_CMD_RECEIVE_FPDMA_QUEUED))
    {
        bHostLbaCmd = TRUE;
    }

    return bHostLbaCmd;
}

BOOL ATA_HostIsWriteCmd(U8 ucCmdCode)
{
    BOOL bWrite = FALSE;
    if ((ucCmdCode == ATA_CMD_WRITE_FPDMA_QUEUED)
        || (ucCmdCode == ATA_CMD_WRITE_DMA) || (ucCmdCode == ATA_CMD_WRITE_DMA_EXT)
        || (ucCmdCode == ATA_CMD_WRITE_SECTOR) || (ucCmdCode == ATA_CMD_WRITE_SECTOR_EXT)
        || (ucCmdCode == ATA_CMD_WRITE_MULTIPLE) || (ucCmdCode == ATA_CMD_WRITE_MULTIPLE_EXT))
    {
        bWrite = TRUE;
    }

    return bWrite;
}

BOOL ATA_HostIsNoneDataCmd(U8 ucCmdCode)
{
    BOOL bNoneDataCmd = FALSE;

    if ((ucCmdCode == ATA_CMD_STANDBY) ||
        (ucCmdCode == ATA_CMD_STANDBY_IMMEDIATE) ||
        (ucCmdCode == ATA_CMD_SET_FEATURES) ||
        (ucCmdCode == ATA_CMD_SET_MULTIPLE_MODE) ||
        (ucCmdCode == ATA_CMD_SECURITY_FREEZE_LOCK)
        )
    {
        bNoneDataCmd = TRUE;
    }

    return bNoneDataCmd;

}

BOOL ATA_HostIsPIOReadCmd(U8 ucCmdCode)
{
    BOOL  bPIOReadCmd = FALSE;

    if ((ucCmdCode == ATA_CMD_IDENTIFY_DEVICE) ||
        (ucCmdCode == ATA_CMD_READ_LOG_EXT) ||
        (ucCmdCode == ATA_CMD_READ_SECTOR) ||
        (ucCmdCode == ATA_CMD_READ_MULTIPLE) ||
        (ucCmdCode == ATA_CMD_READ_SECTOR_EXT) ||
        (ucCmdCode == ATA_CMD_READ_MULTIPLE_EXT)
        )
    {
        bPIOReadCmd = TRUE;
    }

    return bPIOReadCmd;
}

BOOL ATA_HostIsPIOWriteCmd(U8 ucCmdCode)
{
    BOOL  bPIOWriteCmd = FALSE;

    if ((ucCmdCode == ATA_CMD_WRITE_SECTOR) ||
        (ucCmdCode == ATA_CMD_WRITE_MULTIPLE) ||
        (ucCmdCode == ATA_CMD_WRITE_SECTOR_EXT) ||
        (ucCmdCode == ATA_CMD_WRITE_MULTIPLE_EXT) ||
        (ucCmdCode == ATA_CMD_SECURITY_SET_PASSWORD) ||
        (ucCmdCode == ATA_CMD_SECURITY_DISABLE_PASSWORD) ||
        (ucCmdCode == ATA_CMD_SECURITY_UNLOCK)
        )
    {
        bPIOWriteCmd = TRUE;
    }

    return bPIOWriteCmd;
}

void ATA_GetAtaHostCmdNCQH2DFis(char *pRowCmd, U32 ulStartLba, U32 ulSectorCnt, BOOL bRead)
{
    SATA_H2D_REGISTER_FIS *pH2DFis = (SATA_H2D_REGISTER_FIS*)pRowCmd;

    if(TRUE == bRead)
    {
        pH2DFis->Command = ATA_CMD_READ_FPDMA_QUEUED;
    }
    else
    {
        pH2DFis->Command = ATA_CMD_WRITE_FPDMA_QUEUED;
    }

    pH2DFis->LBA7_0 = ulStartLba & 0xFF;
    pH2DFis->LBA15_8 = (ulStartLba >> 8) & 0xFF;
    pH2DFis->LBA23_16 = (ulStartLba >> 16) & 0xFF;
    pH2DFis->LBA31_24 = (ulStartLba >>24) & 0xFF;

    pH2DFis->Feature7_0 = (UCHAR)(ulSectorCnt & 0x000000ff);
    pH2DFis->Feature15_8 = (UCHAR)((ulSectorCnt & 0x0000ff00) >> 8);

    return;
}

void ATA_GetAtaHostCmdDMAH2DFis(char *pRowCmd, U32 ulStartLba, U32 ulSectorCnt, BOOL bRead)
{
    SATA_H2D_REGISTER_FIS *pH2DFis = (SATA_H2D_REGISTER_FIS*)pRowCmd;

    if(TRUE == bRead)
    {
        pH2DFis->Command = ATA_CMD_READ_DMA_EXT;
    }
    else
    {
        pH2DFis->Command = ATA_CMD_WRITE_DMA_EXT;
    }

    pH2DFis->LBA7_0 = ulStartLba & 0xFF;
    pH2DFis->LBA15_8 = (ulStartLba >> 8) & 0xFF;
    pH2DFis->LBA23_16 = (ulStartLba >> 16) & 0xFF;
    pH2DFis->LBA31_24 = (ulStartLba >>24) & 0xFF;

    pH2DFis->Count7_0 = (UCHAR)(ulSectorCnt & 0x000000ff);
    pH2DFis->Count15_8 = (UCHAR)((ulSectorCnt>>8)&0x000000ff);

    return;
}


