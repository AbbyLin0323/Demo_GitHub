/****************************************************************************
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
*****************************************************************************
Filename     : MixVector_HostApp.c                                    
Version      : Ver 1.0                                               
Date         :                                         
Author       : 

Description: 
Windows pattern for MixVector verification in VT3514 PCIE SSD
Modification History:
20140718    Gavin   created
------------------------------------------------------------------------------*/
#include "BaseDef.h"
#include "Disk_Config.h"
#include "model_common.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "MixVector_HostApp.h"
#include "AHCI_HostModelVar.h"
#include "AHCI_ControllerInterface.h"
#include "crc32.h"
#include <time.h>

#if 0
UINT8 *g_pHostDiskA;    // as memory on host side. allocated in AHCI_HostInit
UINT8 *g_pHostDiskB;    // as memory on host side.
UINT8 *g_pHostDiskC;    // as memory on host side.
#endif
unsigned char g_pHostDiskA[(24<<10)+512];    // as memory on host side. allocated in AHCI_HostInit

unsigned char g_pHostDiskB[(128<<20)+512];    // as memory on host side.

unsigned char g_pHostDiskC[(128<<20)+512];    // as memory on host side.
/* global variables related to host command for mix vector simulation */
#define ROUND_TO_NEXT_SSD_PAGE_BOUNDARY( _lba_ ) ( ( _lba_ + 64 ) & ( ~63 ) )

#define SSD_PAGE_SIZE           32768
#define SECTOR_NUM_PER_SSD_PAGE 64
#define SSD_PAGE_MAX_WRITE_NUM  256
#define CMD_NUM_MAX  32
U8              g_ucCmdNum = 32;
HOST_CMD_EXT    g_tHostCmdExtTable[ 32 ];
COMMON_CMD_INFO g_tCommonInfo[ 3 ];
//USHORT          gPGWriteNumTable[ SUB_DISK_C_SIZE / 32768 ] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
USHORT          gPGWriteNumTable[ CE_MAX ][CMD_NUM_MAX];
//U8*             HostMemDiskBA[ 3 ] = { g_pHostDiskA, g_pHostDiskB, g_pHostDiskC };
extern volatile AHCI_PORT *g_pPortHBAReg;

extern void MV_DeviceSchedule(void);
/*  -----------  the end ---------------*/

BOOL MV_CmdSlotIsIdle(U8 ucSlot)
{
    BOOL bIdle;

    bIdle = ((g_pPortHBAReg->CI & (1 << ucSlot)) == 0) ? TRUE : FALSE;

    return bIdle;
}

void MixVectorDataCompareA(U32 ulDeviceAddr, UINT64 ullHostAddr, U32 ulLen)
{
    U32 ulLocalAddr = ulDeviceAddr;
    UINT64 ullHAddr = ullHostAddr;
    U32 ulLength = ulLen;
    while(ulLength--)
    {
        if (*(U8*)(ulLocalAddr++) != *(U8*)(ullHAddr++) )
        {
            printf("Disk A data compare failed .\n");
            DBG_Break();
        }
    }
}

/* by Charles Zhou */
void MV_SendCmd( U8 ucCmdTag )
{
    g_tHostCmdExtTable[ ucCmdTag ].DiskInfo[ 0 ].pHandler( &g_tHostCmdExtTable[ ucCmdTag ], &g_tCommonInfo[ 0 ] );
    g_tHostCmdExtTable[ ucCmdTag ].DiskInfo[ 1 ].pHandler( &g_tHostCmdExtTable[ ucCmdTag ], &g_tCommonInfo[ 1 ] );
    g_tHostCmdExtTable[ ucCmdTag ].DiskInfo[ 2 ].pHandler( &g_tHostCmdExtTable[ ucCmdTag ], &g_tCommonInfo[ 2 ] );

    AHCI_HostCSetPxCI( ucCmdTag );
}

void MV_GenPatternDiskA(HOST_CMD *pHCMD,U32 ulSend, HOST_REQ_TYPE Type)
{
    //    printf ("%d %d %d \n",ERASE_C,WRITE_C,READ_C);
    UINT64 ullHostAddr = 0;
    pHCMD->SubDiskCmd[SUB_DISK_A].DiskEn = TRUE;
    pHCMD->SubDiskCmd[SUB_DISK_A].ReqType = Type;
    pHCMD->SubDiskCmd[SUB_DISK_A].StartUnit = (( ulSend * SEC_SIZE ) % SUB_DISK_A_SIZE);
    pHCMD->SubDiskCmd[SUB_DISK_A].UnitLength = SEC_SIZE;

    ullHostAddr = (UINT64)(g_pHostDiskA) + ((ulSend * SEC_SIZE) % SUB_DISK_A_SIZE);

    pHCMD->SubDiskCmd[SUB_DISK_A].HostAddrHigh = ullHostAddr >> 32; //0;
    pHCMD->SubDiskCmd[SUB_DISK_A].HostAddrLow = ullHostAddr & 0xffffffff; //(((ulSend >> 1)*SEC_SIZE) % SUB_DISK_A_SIZE);    
}

void MV_GenPatternDiskB(HOST_CMD *pHCMD,U32 ulSend, HOST_REQ_TYPE Type )
{
    UINT64 ullHostAddr = 0;
    pHCMD->SubDiskCmd[SUB_DISK_B].DiskEn = TRUE;
    pHCMD->SubDiskCmd[SUB_DISK_B].ReqType = Type;
    pHCMD->SubDiskCmd[SUB_DISK_B].StartUnit = (((ulSend )*SEC_PER_BUF) % (SUB_DISK_B_SIZE/BUF_SIZE));
    pHCMD->SubDiskCmd[SUB_DISK_B].UnitLength = SEC_PER_BUF;

    ullHostAddr = (UINT64)(g_pHostDiskB) + (((ulSend )*SEC_PER_BUF) % (SUB_DISK_B_SIZE/BUF_SIZE));

    pHCMD->SubDiskCmd[SUB_DISK_B].HostAddrHigh = (ullHostAddr >> 32 ) & 0xffffffff; //0;
    pHCMD->SubDiskCmd[SUB_DISK_B].HostAddrLow = (ullHostAddr) & 0xffffffff; //(((ulSend >> 1)*SEC_PER_BUF) % (SUB_DISK_B_SIZE/BUF_SIZE)); 

}
/*
Need to improve regarding Type
*/
void MV_GenPatternDiskC(HOST_CMD *pHCMD,U32 ulSend, HOST_REQ_TYPE Type )
{
    U32 i = 0 ;
    UINT64 ullHostAddr = 0;
    if (ERASE_C == (ulSend & 0x3))
    {
        pHCMD->SubDiskCmd[SUB_DISK_C].DiskEn = TRUE;
        pHCMD->SubDiskCmd[SUB_DISK_C].ReqType = HOST_REQ_ERASE;
        pHCMD->SubDiskCmd[SUB_DISK_C].StartUnit = 0; 
        pHCMD->SubDiskCmd[SUB_DISK_C].UnitLength = CE_NUM;//FLASH_ADDR_CNT; 
        pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrHigh = 0;
        pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrLow = 0; 
        for (i=0;i<CE_NUM;i++)
        {   
            pHCMD->FlashAddrGroup[i].PU = i;
            pHCMD->FlashAddrGroup[i].Block = 0;
            pHCMD->FlashAddrGroup[i].Page = (ulSend >> 2)%(MV_PAGE_PER_BLOCK);
        }         
    }
    else if (WRITE_C == (ulSend & 0x3))
    {
        pHCMD->SubDiskCmd[SUB_DISK_C].DiskEn = TRUE;
        pHCMD->SubDiskCmd[SUB_DISK_C].ReqType = HOST_REQ_WRITE;
        pHCMD->SubDiskCmd[SUB_DISK_C].StartUnit = 0 ;
        pHCMD->SubDiskCmd[SUB_DISK_C].UnitLength = SEC_PER_BUF* CE_NUM;
        //      pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrLow = ((ulSend >> 2)*SEC_PER_BUF* FLASH_ADDR_CNT )%(SUB_DISK_C_SIZE/(BUF_SIZE*FLASH_ADDR_CNT));
        pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrLow = (U32)((UINT64)g_pHostDiskC & 0xffffffff);
        pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrHigh = (U32)(((UINT64)g_pHostDiskC >> 32) & 0xffffffff);
        for (i=0;i<CE_NUM;i++)
        {
            pHCMD->FlashAddrGroup[i].PU = i;
            pHCMD->FlashAddrGroup[i].Block = 0;
            pHCMD->FlashAddrGroup[i].Page = (ulSend >> 2)%(MV_PAGE_PER_BLOCK);                
        }
    }
    else if (READ_C == (ulSend & 0x3))
    {
        pHCMD->SubDiskCmd[SUB_DISK_C].DiskEn = TRUE;
        pHCMD->SubDiskCmd[SUB_DISK_C].ReqType = HOST_REQ_READ;
        pHCMD->SubDiskCmd[SUB_DISK_C].StartUnit = 0 ;
        pHCMD->SubDiskCmd[SUB_DISK_C].UnitLength = SEC_PER_BUF * CE_NUM;
        //      pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrLow = ((ulSend >> 2)*SEC_PER_BUF * FLASH_ADDR_CNT )%(SUB_DISK_C_SIZE/(BUF_SIZE*FLASH_ADDR_CNT));
        pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrLow = (U32)((UINT64)g_pHostDiskC & 0xffffffff);
        pHCMD->SubDiskCmd[SUB_DISK_C].HostAddrHigh = (U32)(((UINT64)g_pHostDiskC >> 32) & 0xffffffff);
        for (i=0;i<CE_NUM;i++)
        {
            pHCMD->FlashAddrGroup[i].PU = i;
            pHCMD->FlashAddrGroup[i].Block = 0;
            pHCMD->FlashAddrGroup[i].Page = (ulSend >> 2)%(MV_PAGE_PER_BLOCK);                
        }
    }
}


void MV_GenPattern(HOST_CMD *pHCMD,U32 ulSend, HOST_REQ_TYPE Type)
{
    HOST_CMD tHCMD = {0};
    MV_GenPatternDiskA(&tHCMD,ulSend, Type );
    //MV_GenPatternDiskB(&tHCMD,ulSend, Type );
    //MV_GenPatternDiskC(&tHCMD,ulSend);
    //   HAL_MemCpy((U32*)pHCMD,(U32*)&tHCMD,sizeof(HOST_CMD));
    *pHCMD = tHCMD;
}


void MixVectorDataCompare(U32 HostAddrHigh, U32 HostAddrLow, U32 LocalAddr, U32 ByteLen, U8 nTag)
{
    UINT64 ullHostAddr = ((UINT64)HostAddrHigh << 32) + (UINT64)HostAddrLow;
    UINT64 ullLocalAddr = (UINT64)LocalAddr;
    U32 ulLen = ByteLen;
    while(ulLen--)
    {
        if (*(U8*)ullHostAddr++ != *(U8*)ullLocalAddr++)
        {
            DBG_Break();
        }
    }
}

void MV_CmdInit()
{
    ULONG LoopNum;
    ULONG i;
    U8 n;

    LoopNum = 1048576;
#if 0    	
    g_pHostDiskA = (UINT8*)malloc(24<<10);
    g_pHostDiskB = (UINT8*)malloc(128<<20);
    g_pHostDiskC = (UINT8*)malloc(128<<20);
#endif
    g_tCommonInfo[ 0 ].DiskIndex        = 0;
    g_tCommonInfo[ 0 ].MemBASw          = g_pHostDiskA;
    g_tCommonInfo[ 0 ].Length           = 512;  // 512 bytes
    g_tCommonInfo[ 0 ].MaxNumPerLoop    = SUB_DISK_A_SIZE / g_ucCmdNum / g_tCommonInfo[ 0 ].Length;
    g_tCommonInfo[ 0 ].MaxLoopNum       = LoopNum;
    g_tCommonInfo[ 0 ].ShiftNum         = 0;

    g_tCommonInfo[ 1 ].DiskIndex        = 1;
    g_tCommonInfo[ 1 ].MemBASw          = g_pHostDiskB;
    g_tCommonInfo[ 1 ].Length           = 65;    // 1 sector
    g_tCommonInfo[ 1 ].MaxNumPerLoop    = ( SUB_DISK_B_SIZE >> 9 ) / g_ucCmdNum / g_tCommonInfo[ 1 ].Length;
    g_tCommonInfo[ 1 ].MaxLoopNum       = LoopNum;
    g_tCommonInfo[ 1 ].ShiftNum         = 9;

    g_tCommonInfo[ 2 ].DiskIndex        = 2;
    g_tCommonInfo[ 2 ].MemBASw          = g_pHostDiskC;
    g_tCommonInfo[ 2 ].Length           = SEC_PER_BUF * CE_NUM;   // 64 sector, 32k bytes.
    g_tCommonInfo[ 2 ].MaxNumPerLoop    = ( SUB_DISK_C_SIZE >> 9 ) / g_ucCmdNum / g_tCommonInfo[ 2 ].Length;
    g_tCommonInfo[ 2 ].MaxLoopNum       = LoopNum;
    g_tCommonInfo[ 2 ].ShiftNum         = 9;

    for( i = 0; i < g_ucCmdNum; i++)
    {
        g_tHostCmdExtTable[ i ].Tag            = (UCHAR)i;
        for( n = 0; n < 3; n++)
        {
            g_tHostCmdExtTable[ i ].DiskInfo[ n ].DiskIndex    = n;
            g_tHostCmdExtTable[ i ].DiskInfo[ n ].StartOffset  = i * g_tCommonInfo[ n ].MaxNumPerLoop * g_tCommonInfo[ n ].Length;
            g_tHostCmdExtTable[ i ].DiskInfo[ n ].CurrentIndex = 0;
            g_tHostCmdExtTable[ i ].DiskInfo[ n ].LoopNum      = 0;
            g_tHostCmdExtTable[ i ].DiskInfo[ n ].MagicNum     = 0;
            g_tHostCmdExtTable[ i ].DiskInfo[ n ].Crc          = 0;
            switch( n )
            {
            case SUB_DISK_A:
            case SUB_DISK_B:
                g_tHostCmdExtTable[ i ].DiskInfo[ n ].pHandler     = WriteDiskAB;
                break;
            case SUB_DISK_C:
                g_tHostCmdExtTable[ i ].DiskInfo[ n ].pHandler     = AsyncEraseFlash;
                break;
            default:
                g_tHostCmdExtTable[ i ].DiskInfo[ n ].pHandler     = NullFunction;
                break;
            }

        }
        memset( (PVOID)&g_tHostCmdExtTable[ i ].HostCmd, 0, sizeof( HOST_CMD ) );
        g_tHostCmdExtTable[ i ].State          = CMD_IDLE;
    }

    //Initial  gPGWriteNumTable
    memset((U32 *)gPGWriteNumTable, 0xFF, (sizeof(USHORT)*CMD_NUM_MAX*CE_NUM));
}

STATUS WriteDiskAB( PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    PDISK_EXT diskInfo;
    U8*  memAddr;
    ULONG   offset;


    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];
    offset   = diskInfo->StartOffset + ( diskInfo->CurrentIndex * CommonInfo->Length );
    memAddr = CommonInfo->MemBASw + ( offset  << CommonInfo->ShiftNum);
    diskInfo->MagicNum = ( diskInfo->LoopNum << 16 ) + (ULONG)( (ULONG_PTR)memAddr >> 16 );
    diskInfo->Crc =  FillPayload(
        (PUCHAR)memAddr,
        ( CommonInfo->Length << CommonInfo->ShiftNum ),
        &diskInfo->MagicNum
        );
    diskInfo->State = CMD_WRITING;
    BuildSubDiskCmd( &g_pMvHCmdTable[ CmdExt->Tag ],
        CommonInfo->DiskIndex,
        HOST_REQ_WRITE,
        offset,
        CommonInfo->Length
        );
    diskInfo->pHandler = ReadDiskAB;
    return CMD_SUCCESS;
}
STATUS ReadDiskAB( PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    ULONG   offset;
    PDISK_EXT diskInfo;
    U8*  memAddr;

    if ( g_pMvHCmdTable[ CmdExt->Tag ].FinishCnt[ CommonInfo->DiskIndex ] != ( CommonInfo->Length << CommonInfo->ShiftNum ) )
    {
        DBG_Break();
    }
    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];
    offset   = diskInfo->StartOffset + ( diskInfo->CurrentIndex * CommonInfo->Length );
    memAddr = CommonInfo->MemBASw + ( offset  << CommonInfo->ShiftNum);
    memset( (void*)memAddr, 0, ( CommonInfo->Length << CommonInfo->ShiftNum ) );
    diskInfo->State = CMD_READING;
    BuildSubDiskCmd( &g_pMvHCmdTable[ CmdExt->Tag ],
        CommonInfo->DiskIndex,
        HOST_REQ_READ,
        offset,
        CommonInfo->Length
        );
    diskInfo->pHandler = CheckDataForDiskAB;
    return CMD_SUCCESS;
}
STATUS CheckDataForDiskAB(PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    ULONG offset;
    STATUS ret;
    PDISK_EXT diskInfo;
    U8*  memAddr;

    if ( g_pMvHCmdTable[ CmdExt->Tag ].FinishCnt[ CommonInfo->DiskIndex ] != ( CommonInfo->Length << CommonInfo->ShiftNum ) )
    {
        DBG_Break();
    }
    DBG_Printf("check disk A/B data of tag %d\n", CmdExt->Tag);
    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];
    offset   = diskInfo->StartOffset + ( diskInfo->CurrentIndex * CommonInfo->Length );
    memAddr = CommonInfo->MemBASw + ( offset  << CommonInfo->ShiftNum);
    diskInfo->MagicNum = ( diskInfo->LoopNum << 16 ) + (ULONG)( (ULONG_PTR)memAddr >> 16 );

    ret = CheckPayload( 
        (PUCHAR)memAddr,
        ( CommonInfo->Length << CommonInfo->ShiftNum ),
        &diskInfo->MagicNum,
        &diskInfo->Crc
        );
    if ( CMD_SUCCESS != ret )
    {
        DBG_Break();
    }

    diskInfo->CurrentIndex++;
    if ( diskInfo->CurrentIndex >= CommonInfo->MaxNumPerLoop )
    {
        diskInfo->CurrentIndex = 0;
        if ( ( diskInfo->LoopNum % 32 ) == 0 )
        {
            DBG_Printf("Cmd %d:  %5d loops have been completed.\b\n", CmdExt->Tag, diskInfo->LoopNum );
        }
        diskInfo->LoopNum++;
        if ( diskInfo->LoopNum >= CommonInfo->MaxLoopNum )
        {
            DBG_Printf("All done for RW disk B!\n");
            diskInfo->LoopNum = 0;
        }
        diskInfo->pHandler = WriteDiskAB;
    }
    else
    {
        diskInfo->pHandler = WriteDiskAB;
    }
    ret = diskInfo->pHandler( CmdExt, CommonInfo );

    return ret;
}

STATUS BuildSubDiskCmd(HOST_CMD *HostCmd, UCHAR DiskIndex, HOST_REQ_TYPE Type, ULONG Offset, ULONG Length )
{
    ULONG i;
    ULONG remainLength;
    ULONG stageLength;  // the length for each FLASH_ADDR entry.
    ULONG currentOffset;
    ULONG posPG;
    ULONGLONG   addr;
    ULONG ulPgPerSlot;
    U16 usBlock;

    addr = (ULONG_PTR)g_tCommonInfo[ DiskIndex ].MemBASw + ( ( DiskIndex == SUB_DISK_A ) ? Offset : ( Offset << 9 ) );

    //    ASSERT( DiskIndex <= SUB_DISK_C );
    //    ASSERT( Type < HOST_REQ_OTHER );
    HostCmd->SubDiskCmd[ DiskIndex ].DiskEn       = 1;
    HostCmd->SubDiskCmd[ DiskIndex ].ReqType      = Type;
    HostCmd->SubDiskCmd[ DiskIndex ].UnitLength   = Length;
    HostCmd->SubDiskCmd[ DiskIndex ].HostAddrLow  = (U32)( addr & 0xFFFFFFFF );
    HostCmd->SubDiskCmd[ DiskIndex ].HostAddrHigh = (U32)( addr >> 32 );
    HostCmd->FinishCnt [ DiskIndex ] = 0;
    HostCmd->ErrFlag = 0;
    if ( SUB_DISK_C == DiskIndex )
    {
        // As for disk C, StartUnit indicates the offset to SSD page boundary, in units of SECTOR.
        if ( ( ( Offset & ( SECTOR_NUM_PER_SSD_PAGE - 1 ) ) + Length )
        > ( SECTOR_NUM_PER_SSD_PAGE * CE_NUM ) )
        {
            return DATA_SIZE_TOO_LONG;
        }

        if ( HOST_REQ_ERASE == Type )
        {
            HostCmd->SubDiskCmd[ DiskIndex ].StartUnit = 0;
            stageLength = SECTOR_NUM_PER_SSD_PAGE;//SSD_PAGE_SIZE;
        }
        else if ( HOST_REQ_WRITE == Type )
        {
            // As for writing disk C, Offset must be align to SSD_PAGE_SIZE.
            if ( Offset & ( SECTOR_NUM_PER_SSD_PAGE - 1 ) )
            {
                return INPUT_PARAMETER_ERROR;
            }
            // StarUnit must be 0.
            HostCmd->SubDiskCmd[ DiskIndex ].StartUnit = 0;
            stageLength = SECTOR_NUM_PER_SSD_PAGE;//SSD_PAGE_SIZE;
        }
        else
        {
            // As for reading disk C, StartUnit might be not align to SSD_PAGE_SIZE.
            HostCmd->SubDiskCmd[ DiskIndex ].StartUnit    = ( Offset & ( SECTOR_NUM_PER_SSD_PAGE - 1 ) );
            stageLength     = ROUND_TO_NEXT_SSD_PAGE_BOUNDARY( Offset ) - Offset;
        }

        remainLength    = Length;
        currentOffset   = Offset;
        posPG = ( Offset / SECTOR_NUM_PER_SSD_PAGE );
        ulPgPerSlot = (SUB_DISK_C_SIZE / (SEC_SIZE * SEC_PER_BUF)) / g_ucCmdNum;

        for( i = 0;( ( i < FLASH_ADDR_CNT ) && ( remainLength > 0 ) ); i++)
        {
            if ( stageLength > remainLength )
            {
                stageLength = remainLength;
            }

            HostCmd->FlashAddrGroup[ i ].ulAsU32   = 0;
            HostCmd->FlashAddrGroup[ i ].PU = posPG % CE_NUM + i% CE_NUM;
            HostCmd->FlashAddrGroup[ i ].Block   = posPG/ulPgPerSlot;//posPG/(PG_PER_BLK * FLASH_ADDR_CNT);
            usBlock = HostCmd->FlashAddrGroup[ i ].Block;
            switch( Type )
            {
            case HOST_REQ_WRITE:
                {
                    if( gPGWriteNumTable[ i ][usBlock] == 0xFFFF )
                    {
                        return SSD_PAGE_NOT_WRITTEN;
                    }
                    else
                    {
                        HostCmd->FlashAddrGroup[ i ].Page    = (U32)gPGWriteNumTable[ i ][usBlock];
                        gPGWriteNumTable[ i ][usBlock]++;
                    }
                }
                break;
            case HOST_REQ_READ:
                if (( gPGWriteNumTable[ i ][usBlock] == 0xFFFF ) 
                    ||  ( gPGWriteNumTable[ i ][usBlock] == 0x0 ))
                {
                    return SSD_PAGE_NOT_WRITTEN;
                }
                else
                {
                    HostCmd->FlashAddrGroup[ i ].Page    = (U32)gPGWriteNumTable[ i ][usBlock] - 1;
                    if((U32)gPGWriteNumTable[ i ][usBlock] >= PG_PER_BLK)
                    {
                        gPGWriteNumTable[ i ][usBlock] = 0;
                    }
                }
                break;
            case HOST_REQ_ERASE:
                HostCmd->SubDiskCmd[ DiskIndex ].UnitLength = CE_NUM;
                HostCmd->FlashAddrGroup[ i ].Page    = 0; 
                gPGWriteNumTable[ i ][usBlock] = 0x0;
                break;
            default:
                break;
            }

            //LBA2FlashAddr( currentOffset, &HostCmd->FlashAddrGroup[ i ] );
            remainLength  -= stageLength;
            currentOffset += stageLength;
            stageLength    = SECTOR_NUM_PER_SSD_PAGE;
        }
    }
    else
    {
        // As for Disk A/B
        HostCmd->SubDiskCmd[ DiskIndex ].StartUnit    = Offset;
    }

    return CMD_SUCCESS;
}

STATUS AsyncEraseFlash( PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    ULONG offset;
    PDISK_EXT diskInfo;

    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];

    offset              = diskInfo->StartOffset + ( diskInfo->CurrentIndex * CommonInfo->Length );
    diskInfo->State     = CMD_IDLE;
    diskInfo->pHandler  = AsyncWriteFlash;

    BuildSubDiskCmd( &g_pMvHCmdTable[ CmdExt->Tag ],
        SUB_DISK_C,
        HOST_REQ_ERASE,
        offset,
        CommonInfo->Length
        );
    return CMD_SUCCESS;
}

//STATUS AsyncEraseFlash( PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
//{
//    U8 ucBlockMax;
//    U8 ucPuIndex;
//    PDISK_EXT diskInfo;
//    static U8 ucCurBlockIndex = 0;
//    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];
//    diskInfo->State     = CMD_IDLE;
//
//    g_pMvHCmdTable->SubDiskCmd[SUB_DISK_C].DiskEn = TRUE;
//    g_pMvHCmdTable->SubDiskCmd[SUB_DISK_C].ReqType = HOST_REQ_ERASE;
//    g_pMvHCmdTable->SubDiskCmd[SUB_DISK_C].UnitLength = FLASH_ADDR_CNT;
//    ucBlockMax = SUB_DISK_C_SIZE/(PG_PER_BLK * PG_SZ * PLN_PER_PU * CE_NUM);
//
//    for(ucPuIndex = 0; ucPuIndex < FLASH_ADDR_CNT; ucPuIndex++)
//    {
//        g_pMvHCmdTable->FlashAddrGroup[ucPuIndex].PU = ucPuIndex;
//        g_pMvHCmdTable->FlashAddrGroup[ucPuIndex].Block = ucCurBlockIndex;
//        gPGWriteNumTable[ucPuIndex][ucCurBlockIndex] = 0;
//    }
//
//    ucCurBlockIndex++;
//    if(ucCurBlockIndex >= ucBlockMax)
//    {
//        ucCurBlockIndex = 0;
//        diskInfo->pHandler  = AsyncWriteFlash;
//    }
//    else
//    {
//        diskInfo->pHandler  = AsyncEraseFlash;
//    }   
//}

STATUS AsyncWriteFlash( PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    ULONG offset;
    U8* memAddr;
    STATUS ret;
    PDISK_EXT diskInfo;

    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];

    offset = diskInfo->StartOffset + ( diskInfo->CurrentIndex * CommonInfo->Length );
    memAddr = CommonInfo->MemBASw + ( offset << CommonInfo->ShiftNum );
    diskInfo->MagicNum  = ( diskInfo->LoopNum << 16 ) + (ULONG)( (ULONG_PTR)memAddr >> 16 );
    diskInfo->State     = CMD_WRITING;
    diskInfo->pHandler  = AsyncReadFlash;
    diskInfo->Crc       = FillPayload(
        (PUCHAR)memAddr,
        ( CommonInfo->Length << CommonInfo->ShiftNum ),
        &diskInfo->MagicNum
        );
    ret = BuildSubDiskCmd( &g_pMvHCmdTable[ CmdExt->Tag ],
        SUB_DISK_C,
        HOST_REQ_WRITE,
        offset,
        CommonInfo->Length
        );
    return ret;
}

STATUS AsyncReadFlash( PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    ULONG offset;
    STATUS ret;
    PDISK_EXT diskInfo;

    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];
    offset = diskInfo->StartOffset + ( diskInfo->CurrentIndex * CommonInfo->Length );
    diskInfo->State     = CMD_READING;
    diskInfo->pHandler  = CheckDataForReadFlash;

    ret = BuildSubDiskCmd( &g_pMvHCmdTable[ CmdExt->Tag ],
        SUB_DISK_C,
        HOST_REQ_READ,
        offset,
        CommonInfo->Length
        );
    return ret;
}
/*
* Advance state from CheckData to Write or Erase.
*/
STATUS CheckDataForReadFlash(PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    ULONG offset;
    U8* memAddr;
    STATUS ret;

    PDISK_EXT diskInfo;

    diskInfo = &CmdExt->DiskInfo[ CommonInfo->DiskIndex ];
    offset  = diskInfo->StartOffset + ( diskInfo->CurrentIndex * CommonInfo->Length );
    memAddr = CommonInfo->MemBASw + ( offset << CommonInfo->ShiftNum );
    ret = CheckPayload( 
        (PUCHAR)memAddr,
        ( CommonInfo->Length << CommonInfo->ShiftNum ),
        &diskInfo->MagicNum,
        &diskInfo->Crc
        );

    diskInfo->CurrentIndex++;
    if ( diskInfo->CurrentIndex >= CommonInfo->MaxNumPerLoop )
    {
        diskInfo->CurrentIndex = 0;
        diskInfo->LoopNum++;
        if((diskInfo->LoopNum % 16) == 0)
        {
            printf("Cmd tag; Num in Loop: %d %d\n", CmdExt->Tag, diskInfo->LoopNum);
        }
        if ( diskInfo->LoopNum >= CommonInfo->MaxLoopNum )
        {
            diskInfo->pHandler = NullFunction;
        }
        else
        {
            diskInfo->pHandler = AsyncEraseFlash;
        }
    }
    else
    {
        if(0 == diskInfo->CurrentIndex%PG_PER_BLK)
        {
            diskInfo->pHandler = AsyncEraseFlash;
        }
        else
        {
            diskInfo->pHandler = AsyncWriteFlash;
        }
    }
    ret = diskInfo->pHandler( CmdExt, CommonInfo );

    return ret;
}
STATUS NullFunction( PHOST_CMD_EXT CmdExt, PCOMMON_CMD_INFO CommonInfo )
{
    return LOOP_COMPLETED;
}

#define SOF   0x78563412
ULONG FillPayload( PUCHAR Buffer, ULONG Size, PULONG pMagicNum)
{
    ULONG i;
    ULONG ret;
    //ASSERT( Buffer );
    //ASSERT( Size > 0 );
    srand( (ULONG)time( NULL ) );
    if ( Size == 0 )
    {
        return 0;
    }
    if ( Size < 12 )
    {
        for( i = 0; i < Size; i++)
        {
            Buffer[ i ] = (UCHAR)( rand() & 0xFF );
        }
        ret = crc32( 0, Buffer, Size );
    }else{
        if ( pMagicNum )
        {
            *(PULONG)( Buffer )     = *pMagicNum;
        }else{
            *(PULONG)( Buffer )     = SOF;
        }
        *(PULONG)( Buffer + 4 ) = Size;
        for( i = 0; i < ( Size - 12 ); i++)
        {
            Buffer[ i + 8 ] = (UCHAR)( rand() & 0xFF );
        }
        ret = crc32( 0, Buffer + 8, ( Size - 12 ) );
        *(PULONG)( Buffer + Size - 4 ) = ret;
    }
    return ret;
}
STATUS CheckPayload( PUCHAR Buffer, ULONG Size, PULONG pMagicNum, PULONG Crc )
{
    ULONG crc;

    //	ASSERT( Buffer );
    //	ASSERT( Size > 0 );

    if ( Size >= 12 )
    {
        crc = ( pMagicNum == NULL ) ? SOF : (*pMagicNum );
        if ( *(PULONG)( Buffer ) != crc )
        {
            return CMD_UNSUCCESSFUL;
        }
        if ( *(PULONG)( Buffer + 4 ) != Size )
        {
            return CMD_UNSUCCESSFUL;
        }
        crc = crc32( 0, Buffer + 8, Size - 12 );
        if ( *(PULONG)( Buffer + Size - 4 ) != crc )
        {
            return CMD_UNSUCCESSFUL;
        }
        if ( Crc != NULL )
        {
            if ( crc != *Crc )
            {
                return CMD_UNSUCCESSFUL;
            }
        }
    }else{
        if ( Size > 0 )
        {
            crc = crc32( 0, Buffer, Size );
            if ( crc != *Crc )
            {
                return CMD_UNSUCCESSFUL;
            }
        }
    }
    return CMD_SUCCESS;
}
/*
by Charles Zhou

Get the transfer length of specified command (R/W disk B).
for mix vector
*/
U32 MV_GetHostCmdDiskBLbaLength( U8 ucCmdTag )
{
    if (FALSE == g_pMvHCmdTable[ ucCmdTag ].SubDiskCmd[ 1 ].DiskEn)
    {
        return 0;
    }
    else
    {
        return g_pMvHCmdTable[ ucCmdTag ].SubDiskCmd[ 1 ].UnitLength;
    }
}

U32 MV_GetHostCmdDiskCLbaLength( U8 ucCmdTag )
{
    if (FALSE == g_pMvHCmdTable[ ucCmdTag ].SubDiskCmd[ 2 ].DiskEn
        || HOST_REQ_ERASE == g_pMvHCmdTable[ ucCmdTag ].SubDiskCmd[ 2 ].ReqType)
    {
        return 0;
    }
    else
    {
        return g_pMvHCmdTable[ ucCmdTag ].SubDiskCmd[ 2 ].UnitLength;
    }
}


