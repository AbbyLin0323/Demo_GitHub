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
#ifndef _MAIN_H_
#define _MAIN_H_
#define DOS_ENV
#ifdef DOS_ENV
#include "ahci.h"

#include "TraceLog.h"
#endif


/*****************************************************************************
 *
 *  Debug Functions
 *
 ****************************************************************************/
#define LOG_FILE_NAME   "ahci_log.txt"

#define DBG_ERROR              0
#define DBG_WARNING            1
#define DBG_NORMAL             2
#define DBG_TRACE              3
#define DBG_FUNCTION           3

#define COMP_INIT            0x00000001
#define COMP_SCRIPT          0x00000002
#define COMP_ADD             0x00000004
#define COMP_REMOVE          0x00000008
#define COMP_CRING           0x00000010
#define COMP_ERING           0x00000020
#define COMP_TRING           0x00000040
#define COMP_RING            ( COMP_CRING | COMP_ERING | COMP_TRING )
#define COMP_MEMMGT          0x00000100
#define COMP_HW              0x00000200
#define COMP_XHC             0x00000400
#define COMP_SLOT            0x00000800
#define COMP_EP              0x00001000
#define COMP_ALL             0xFFFFFFFF

#define COMP_CNT 14

typedef struct _pci_dev_info{
    UCHAR      BusNum;
    UCHAR      DevNum;
    UCHAR      FuncNum;
    WORD       DeviceID;
    WORD       VendorID;
}PCI_DEV_INFO, *PPCI_DEV_INFO;

#define CMD_TABLE_HW_ADDRESS    ( 32 * 1048576 )
#define DISK_A_HW_ADDRESS       ( 33 * 1048576 )
#define CMD_TABLE_SIZE          ( 1  * 1048576 )
#define DISK_A_SIZE             ( 1  * 1048576 )

#define DISK_B_HW_ADDRESS       ( 128 * 1048576 )
#define DISK_C_HW_ADDRESS       ( 256 * 1048576 )
#define DISK_BC_SIZE            ( 256 * 1048576 )
#define DISK_B_SIZE             ( 128 * 1048576 )
#define DISK_C_SIZE             ( 128 * 1048576 )


#define SECTOR_SZ  512
#define SECTOR_SZ_BIT 9
#define ROUND_TO_NEXT_SSD_PAGE_BOUNDARY( _lba_ ) ( ( _lba_ + 63 ) & ( ~63 ) )

//#define PU_NUM                  8
#define SSD_PAGE_SIZE           32768
#define SECTOR_NUM_PER_SSD_PAGE 64
#define SSD_PAGE_MAX_WRITE_NUM  256

#define VIA_AHCI_VENDOR_ID 0x1106
#define VIA_AHCI_DEVICE_ID 0x0514

/*--------------------------- interface with host ----------------------------*/
#define SUB_DISK_CNT 3
#define SUB_DISK_A 0
#define SUB_DISK_B 1
#define SUB_DISK_C 2
#define FLASH_ADDR_CNT 32
#define MAX_CMD_SLOT 32

typedef enum _host_cmd_stat
{
    INIT_CMD = 0,
    GET_CMD_SLOT,
    TRIG_CMD,
    WAIT_CMD,
    CHECK_CMD
}HOST_CMD_STAT;


typedef enum _sub_cmd_type
{
    DISK_A = 0,
    DISK_B,
    DISK_C
}SUB_CMD_TYPE;
typedef enum _disk_cmd_status{

#if 1
    DISK_CMD_SUCCESS,
    RD_DISK_FINISH,
    RD_DISK_CMD_SEND_FINISH,
    WT_DISK_FINISH,
    WT_DISK_CMD_SEND_FINISH
#endif
}DISK_CMD_STATUS;


typedef enum _HOST_REQ_TYPE
{
    HOST_REQ_READ = 0,
    HOST_REQ_WRITE,
    HOST_REQ_ERASE,
    HOST_REQ_OTHER,
    HOST_REQ_INVALID
}HOST_REQ_TYPE;

typedef struct _SUB_DISK_CMD
{
    U32 DiskEn: 1;
    U32 ReqType: 2;
    U32 SplitEnable:1;
    U32 Dw0Rsvd: 4;
    U32 StartUnit: 24;//for diskA, unit means byte; for read/write diskB/C, means sector
    U32 UnitLength;//
    U32 HostAddrLow;
    U32 HostAddrHigh;
}SUB_DISK_CMD;

typedef union _FLASH_ADDR
{
    struct {
    U32 PU: 5;
    U32 Block: 11;
    U32 Page: 9;
    U32 Rsvd: 7;
    };
    U32 AsU32;
}FLASH_ADDR, *PFLASH_ADDR;

typedef struct _HOST_CMD
{
    SUB_DISK_CMD SubDiskCmd[SUB_DISK_CNT];//dword 0~11

    FLASH_ADDR FlashAddrGroup[FLASH_ADDR_CNT];//dword 12~43

    ULONG FinishCnt[SUB_DISK_CNT];//dword 44~46

    UCHAR ErrFlag[ 4 ];//dword 47

    USHORT HsgLength[64];//dword 48~79
}HOST_CMD, *PHOST_CMD;


typedef struct _ahci_ext{
    PCI_DEV_INFO    PciDevInfo;
    ULONG           HBARegBaseAddress;
    ULONG*          CLBRegAddress;
    ULONG*          FBRegAddress;
    ULONG*          PxCIRegAddress;

    PHOST_CMD       HostCmdTable;
    UCHAR *           DiskAAddrSw;//linerar address for host fill payload and check data
    UCHAR *           DiskBAddrSw;
    UCHAR *           DiskCAddrSw;
    DWORD           PxCI;

}AHCI_EXT, *PAHCI_EXT;

void    main(int argc, char**argv);
STATUS  Initialize(int argc, char**argv);
STATUS CheckViaAhciDevice( PPCI_DEV_INFO PciDevInfo );
void    InitAhciExt( PAHCI_EXT AhciExt );
STATUS  SwInit( PAHCI_EXT AhciExt );
STATUS  HwInit( PAHCI_EXT AhciExt );

void    Terminate();
STATUS  HwTerminate( PAHCI_EXT AhciExt );
STATUS  SwTermiate( PAHCI_EXT AhciExt );

void usage(char** argv);
UCHAR ParseOpt( int argc, char** argv );

STATUS  SyncRWEDisk( PAHCI_EXT AhciExt, UCHAR DiskIndex, HOST_REQ_TYPE Type, UCHAR Tag, ULONG Offset, ULONG Length, PULONG DataCrc );
STATUS  BuildSubDiskCmd(PHOST_CMD HostCmd, UCHAR DiskIndex, HOST_REQ_TYPE Type, ULONG Offset, ULONG Length );
void    LBA2FlashAddr( ULONG LBA, PFLASH_ADDR pFlashAddr );
PCHAR   CmdToString( HOST_REQ_TYPE Type );


#endif // _MAIN_H
