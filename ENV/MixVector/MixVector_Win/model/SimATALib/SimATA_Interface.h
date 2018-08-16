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

#ifndef _SIMATA_INTERFACE_H
#define _SIMATA_INTERFACE_H

#define ATA_MAX_SECTOR_CNT 65536 
typedef struct _SATA_H2D_REGISTER_FIS {
    U8 FisType;  // 0x27
    U8 PMPort    :4;
    U8 Reserved1 :3;
    U8 C         :1;     //This bit is set to one when the register transfer is due to an update of the Command register. The bit is set to zero when the register transfer is due to an update of the Device Control register.
    U8 Command;
    union {
        U8 Features;
        U8 Feature7_0;
    };

    union {
        U8 SectorNumber;
        U8 LBA7_0;
    };
    union {
        U8 CylLow;
        U8 LBA15_8;
    };
    union {
        U8 CylHigh;
        U8 LBA23_16;
    };
    union {
        U8 Dev_Head;
        U8 Device;
    };

    union {
        U8 SecNum_Exp;
        U8 LBA31_24;
    };
    union {
        U8 CylLow_Exp;
        U8 LBA39_32;
    };
    union {
        U8 CylHigh_Exp;
        U8 LBA47_40;
    };
    union {
        U8 Features_Exp;
        U8 Feature15_8;
    };

    union {
        U8 SectorCount;
        U8 Count7_0;
    };
    union {
        U8 SectorCount_Exp;
        U8 Count15_8;
    };
    U8 ICC;
    U8 Control;

    U8 Auxiliary7_0;
    U8 Auxiliary15_8;
    U8 Auxiliary23_16;
    U8 Auxiliary31_24;
    U32 Resv[11];

} SATA_H2D_REGISTER_FIS, *PSATA_H2D_REGISTER_FIS;


U32 ATA_HostGetCmdLba(PSATA_H2D_REGISTER_FIS pCFis);
U32 ATA_HostGetNCQSecCnt(PSATA_H2D_REGISTER_FIS pCFis);
U32 ATA_HostGetLBA48SecCnt(PSATA_H2D_REGISTER_FIS pCFis);
U32 ATA_HostGetLBA28SecCnt(PSATA_H2D_REGISTER_FIS pCFis);
U32 ATA_HostGetLbaCmdSecCnt(PSATA_H2D_REGISTER_FIS pCFis);
BOOL ATA_IsNCQCMD(U8 ucCmdCode);
BOOL ATA_HostIsLbaCmd(U8 ucCmdCode);
BOOL ATA_HostIsWriteCmd(U8 ucCmdCode);
BOOL ATA_HostIsPIOWriteCmd(U8 ucCmdCode);
BOOL ATA_HostIsPIOReadCmd(U8 ucCmdCode);
BOOL ATA_HostIsNoneDataCmd(U8 ucCmdCode);

#endif