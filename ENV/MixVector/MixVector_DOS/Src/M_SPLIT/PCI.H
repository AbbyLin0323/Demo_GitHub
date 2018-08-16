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

#ifndef __PCI_H__INCLUDED__
#define __PCI_H__INCLUDED__

#ifndef __PCI_TYPES_H__INCLUDED__
//#include "pcitypes.h"
#endif

//
//  Defines for PCI stuff
//

// Values for GetLastPciError()
#define PCI_ERR_SUCCESS     0x00        // No error (success)
#define PCI_ERR_UNSUPP      0x81        // Unsupported function
#define PCI_ERR_BADVENID    0x83        // Bad vendor id
#define PCI_ERR_NODEV       0x86        // Device not found
#define PCI_ERR_BADREG      0x87        // Bad register number

// Values for BIOS functions
#define PCI_INT             0x1A        // PCI BIOS Interrupt
#define PCI_SIGNATURE       ' ICP'      // Signature for INSTALL call
#define PCI_DEV_MAX         0x1F        // Max. value for dev parameter
#define PCI_FUNC_MAX        0x07        // Max. value for func parameter
#define PCI_BIOS_INSTALL    0xB101      // PCI Bios installation check
#define PCI_BIOS_FINDDEV    0xB102      // PCI Bios find device
#define PCI_BIOS_FINDCLASS  0xB103      // PCI Bios find class
#define PCI_BIOS_CFG_RD8    0xB108      // PCI Bios Read configuration byte
#define PCI_BIOS_CFG_RD16   0xB109      // PCI Bios Read configuration word
#define PCI_BIOS_CFG_RD32   0xB10A      // PCI Bios Read configuration dword
#define PCI_BIOS_CFG_WR8    0xB10B      // PCI Bios Write configuration byte
#define PCI_BIOS_CFG_WR16   0xB10C      // PCI Bios Write configuration word
#define PCI_BIOS_CFG_WR32   0xB10D      // PCI Bios Write configuration dword

// Values for Configuration registers
//  Generic Registers (For all devices)
#define PCI_CFG_R16_VENID   0x00        // Vendor ID register
#define PCI_CFG_R16_DEVID   0x02        // Device ID register
#define PCI_CFG_R16_CMD     0x04        // Command register
#define PCI_CFG_R16_STATUS  0x06        // Status register
#define PCI_CFG_R16_REV     0x08        // Revision ID register
#define PCI_CFG_R24_CLASS   0x09        // Class code
#define PCI_CFG_R8_HDRTYPE  0x0E        // Class code

//  Registers for device with type 0 header
//  There are may more registers, refer to PCI specification
#define PCI_CFG_R32_BAR0    0x10        // BAR0
#define PCI_CFG_R32_BAR1    0x14        // BAR1
#define PCI_CFG_R32_BAR2    0x18        // BAR2
#define PCI_CFG_R32_BAR3    0x1C        // BAR3
#define PCI_CFG_R32_BAR4    0x20        // BAR4
#define PCI_CFG_R32_BAR5    0x24        // BAR5
#define PCI_CFG_R32_CISPTR  0x28        // Cardbus CIS Pointer
#define PCI_CFG_R16_SVID    0x2C        // Subsystem vendor id
#define PCI_CFG_R16_SDID    0x2E        // Subsystem device id
#define PCI_CFG_R32_EXROM   0x30        // Expansion ROM base address
#define PCI_CFG_R8_CAPPTR   0x34        // Capabilities ptr
#define PCI_CFG_R8_INTLINE  0x3C        // Interrupt line
#define PCI_CFG_R8_INTPIN   0x3D        // Interrupt pin

//
//  PCI BIOS generic interfaces
//
UCHAR PciGetLastError();
UCHAR IsPciBiosPresent();
UCHAR GetPciBiosVerLo();
UCHAR GetPciBiosVerHi();
UCHAR GetPciBussesInSystem();
USHORT GetPciDeviceCount();
UCHAR PciSearchForDevice(USHORT venid, USHORT devid, UCHAR* bus, UCHAR* dev, UCHAR* func, USHORT* marker);

//
//  PCI configuration registers read access
//
UCHAR PciReadConfigByte(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg);
USHORT PciReadConfigWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg);
UINT PciReadConfigDWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg);

//
//  PCI configuration registers write access
//
UCHAR PciWriteConfigByte(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, UCHAR data);
UCHAR PciWriteConfigWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, USHORT data);
UCHAR PciWriteConfigDWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, UINT data);

#endif
