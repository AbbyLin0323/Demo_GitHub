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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "DosBaseType.h"
#ifdef DOS_ENV
#include <i86.h>
#include "mmain.h"
#endif

//#include "DosBaseType.h"
#include "pci.h"


#define pci_true  1
#define pci_false 0
//
// Copyright (c) 2004, 2006 Philipp Diethelm
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software, to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
// do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//
//  pci_types.h
//  Version: 1.1
//
//  Version history
//  1.0  23.01.2006  Initial release
//  1.1  30.01.2006  Changed return type of IsPciBiosPresent() to UCHAR
//  1.2  30.01.2006  Added PciSearchForDevice()
//  1.3  31.01.2006  Updated code to uint??_t datatypes
//



//
// Private variable - Read through PciGetLastError()
//
static UCHAR pciLastError = 0;

//
//  PciGetLastError()
//
//  Input:  Nothing
//
//  Return: Last error (AH) from a BIOS call
//

UCHAR PciGetLastError()
{
        return pciLastError;
}

//
//  IsPciBiosPresent
//
//  Input:  Nothing
//
//  Return: TRUE if PCI Bios is present
//          FALSE if PCI Bios not present
//
UCHAR IsPciBiosPresent()
{
#ifdef DOS_ENV
        union REGS regs;

        regs.w.ax = PCI_BIOS_INSTALL;
        regs.x.edi = 0;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;

        if((regs.h.ah==PCI_ERR_SUCCESS) && (regs.x.edx==PCI_SIGNATURE))
                return pci_true;

        return pci_false;
#else
		return pci_true;
#endif
}

//
//  GetPciBiosVerLo()
//
//  Input:  Nothing
//
//  Return: PCI Bios Minor Version
//
UCHAR GetPciBiosVerLo()
{
#ifdef DOS_ENV
        union REGS regs;

        regs.w.ax = PCI_BIOS_INSTALL;
        regs.x.edi = 0;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;

        if((regs.h.ah==PCI_ERR_SUCCESS) && (regs.x.edx==PCI_SIGNATURE))
                return regs.h.bl;

        return 0;
#else
		return 0;
#endif
}

//
//  GetPciBiosVerHi()
//
//  Input:  Nothing
//
//  Return: PCI Bios Major Version
//
UCHAR GetPciBiosVerHi()
{
#ifdef DOS_ENV
        union REGS regs;

        regs.w.ax = PCI_BIOS_INSTALL;
        regs.x.edi = 0;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;

        if((regs.h.ah==PCI_ERR_SUCCESS) && (regs.x.edx==PCI_SIGNATURE))
                return regs.h.bh;

        return 0;
#else
		return 0;
#endif
}

//
//  GetPciBussesInSystem()
//
//  Input:  Nothing
//
//  Return: PCI Bus count
//
UCHAR GetPciBussesInSystem()
{
#ifdef DOS_ENV
        union REGS regs;

        regs.w.ax = PCI_BIOS_INSTALL;
        regs.x.edi = 0;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;

        if((regs.h.ah==PCI_ERR_SUCCESS) && (regs.x.edx==PCI_SIGNATURE))
                return regs.h.cl;

        return 0;
#else
		return 0;
#endif
}

//
//  GetPciDeviceCount()
//
//  Input:  Nothing
//
//  Return: total PCI device count
//
USHORT GetPciDeviceCount()
{
#ifdef DOS_ENV
        union REGS regs;
        USHORT numDev = 0;
        UCHAR  bus;
        UCHAR  dev;
        UCHAR  func;

        for(bus=0; bus<=GetPciBussesInSystem(); bus++)
        {
                for(dev=0; dev<=PCI_DEV_MAX; dev++)
                {
                        // Check if device present
                        if(0xFFFF!=PciReadConfigWord(bus, dev, 0, PCI_CFG_R16_VENID))
                        {
                                UCHAR ucHeaderType = PciReadConfigByte(bus, dev, 0, PCI_CFG_R8_HDRTYPE);
                                ucHeaderType &= 0x80;

                                for(func=0; func<=PCI_FUNC_MAX; func++)
                                {
                                        if(0xFFFF != PciReadConfigWord(bus, dev, func, 0))
                                        {
                                                if(ucHeaderType==0 && func==0)
                                                        numDev++;
                                                else if(ucHeaderType==0x80)
                                                        numDev++;
                                        }
                                }
                        }
                }
        }

        return numDev;
#else
		return 0;
#endif
}

//
//  PciReadConfigByte()
//
//  Input:  bus  - PCI bus number
//          dev  - PCI device number
//          func - PCI function number
//          reg  - PCI configuration register
//
//  Return: configuration register data
//
UCHAR PciReadConfigByte(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg)
{
#ifdef DOS_ENV
        union REGS regs;
        UCHAR devfunc = 0;

        // Mask the input values
        dev  &= PCI_DEV_MAX;
        func &= PCI_FUNC_MAX;

        // Make the conbined dev/func parameter
        devfunc = dev;
        devfunc <<= 3;
        devfunc |= func;

        // Call the BIOS
        regs.w.ax = PCI_BIOS_CFG_RD8;
        regs.h.bh = bus;
        regs.h.bl = devfunc;
        regs.w.di = reg;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;
        return regs.h.cl;
#else
		return 0;
#endif
}

//
//  PciReadConfigWord()
//
//  Input:  bus  - PCI bus number
//          dev  - PCI device number
//          func - PCI function number
//          reg  - PCI configuration register
//
//  Return: configuration register data
//
USHORT PciReadConfigWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg)
{
#ifdef DOS_ENV
        union REGS regs;
        UCHAR devfunc = 0;

        // Mask the input values
        dev  &= PCI_DEV_MAX;
        func &= PCI_FUNC_MAX;

        // Make the conbined dev/func parameter
        devfunc = dev;
        devfunc <<= 3;
        devfunc |= func;

        // Call the BIOS
        regs.w.ax = PCI_BIOS_CFG_RD16;
        regs.h.bh = bus;
        regs.h.bl = devfunc;
        regs.w.di = reg;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;
        return regs.w.cx;
#else
		return 0;
#endif
}

//
//  PciReadConfigDWord()
//
//  Input:  bus  - PCI bus number
//          dev  - PCI device number
//          func - PCI function number
//          reg  - PCI configuration register
//
//  Return: configuration register data
//
/*
INT 1A - Intel PCI BIOS v2.0c - READ CONFIGURATION DWORD
           AX = B10Ah
           BH = bus number
           BL = device/function number (bits 7-3 device, bits 2-0 function)
           DI = register number (0000h-00FFh)

   Return: CF clear if successful
           ECX = dword read
           CF set on error
           AH = status
               00h successful
               87h bad register number
           EAX, EBX, ECX, and EDX may be modified
           all other flags (except IF) may be modified

    Notes: this function may require up to 1024 byte of stack; it will not enable
           interrupts if they were disabled before making the call the meanings of
           BL and BH on return were exchanged between the initial drafts of the
           specification and final implementation
*/
/*
HBA Configuration Registers
This section details how the PCI Header and PCI Capabilities should be constructed for an AHCI HBA.
The fields shown are duplicated from the appropriate PCI specifications. The PCI documents are the
normative specifications for these registers and this section details additional requirements for an AHCI
HBA.
Start (hex)  End (hex)  Name
00 3F PCI Header
PMCAP PMCAP+7 PCI Power Management Capability
MSICAP MSICAP+9 Message Signaled Interrupt Capability
2.1 PCI Header
Start (hex)  End (hex)  Symbol  Name
00 03 ID Identifiers
04 05 CMD Command Register
06 07 STS Device Status
08 08 RID Revision ID
09 0B CC Class Codes
0C 0C CLS Cache Line Size
0D 0D MLT Master Latency Timer
0E 0E HTYPE Header Type
0F 0F BIST Built In Self Test (Optional)
10 23 BARS Other Base Address Registres (Optional) <BAR0-4>
24 27 ABAR AHCI Base Address <BAR5>
2C 2F SS Subsystem Identifiers
30 33 EROM Expansion ROM Base Address (Optional)
34 34 CAP Capabilities Pointer
3C 3D INTR Interrupt Information
3E 3E MGNT Min Grant (Optional)
3F 3F MLAT Max Latency (Optional)
*/
//reg=0x24=nbar (24 27 ABAR AHCI Base Address <BAR5> )
UINT PciReadConfigDWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg)
{
#ifdef DOS_ENV
        union REGS regs;
        UCHAR devfunc = 0;

        // Mask the input values
        dev  &= PCI_DEV_MAX;
        func &= PCI_FUNC_MAX;

        // Make the conbined dev/func parameter
        devfunc = dev;
        devfunc <<= 3;
        devfunc |= func;

        // Call the BIOS
        regs.w.ax = PCI_BIOS_CFG_RD32;
        regs.h.bh = bus;
        regs.h.bl = devfunc;
        regs.w.di = reg;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;
        return regs.x.ecx;
#else
		return 0;
#endif
}

//
//  PciWriteConfigByte()
//
//  Input:  bus  - PCI bus number
//          dev  - PCI device number
//          func - PCI function number
//          reg  - PCI configuration register
//          data - Data to write to register
//
//  Return: pciLastError
//
UCHAR PciWriteConfigByte(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, UCHAR data)
{
#ifdef DOS_ENV
        union REGS regs;
        UCHAR devfunc = 0;

        // Mask the input values
        dev  &= PCI_DEV_MAX;
        func &= PCI_FUNC_MAX;

        // Make the conbined dev/func parameter
        devfunc = dev;
        devfunc <<= 3;
        devfunc |= func;

        // Call the BIOS
        regs.w.ax = PCI_BIOS_CFG_WR8;
        regs.h.bh = bus;
        regs.h.bl = devfunc;
        regs.h.cl = data;
        regs.w.di = reg;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;
        return pciLastError;
#else
		return 0;
#endif
}

//
//  PciWriteConfigWord()
//
//  Input:  bus  - PCI bus number
//          dev  - PCI device number
//          func - PCI function number
//          reg  - PCI configuration register
//          data - Data to write to register
//
//  Return: pciLastError
//
UCHAR PciWriteConfigWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, USHORT data)
{
#ifdef DOS_ENV
        union REGS regs;
        UCHAR devfunc = 0;

        // Mask the input values
        dev  &= PCI_DEV_MAX;
        func &= PCI_FUNC_MAX;

        // Make the conbined dev/func parameter
        devfunc = dev;
        devfunc <<= 3;
        devfunc |= func;

        // Call the BIOS
        regs.w.ax = PCI_BIOS_CFG_WR16;
        regs.h.bh = bus;
        regs.h.bl = devfunc;
        regs.w.cx = data;
        regs.w.di = reg;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;
        return pciLastError;
#else
		return 0;
#endif
}

//
//  PciWriteConfigDWord()
//
//  Input:  bus  - PCI bus number
//          dev  - PCI device number
//          func - PCI function number
//          reg  - PCI configuration register
//          data - Data to write to register
//
//  Return: pciLastError
//
UCHAR PciWriteConfigDWord(UCHAR bus, UCHAR dev, UCHAR func, UCHAR reg, UINT data)
{
#ifdef DOS_ENV
        union REGS regs;
        UCHAR devfunc = 0;

        // Mask the input values
        dev  &= PCI_DEV_MAX;
        func &= PCI_FUNC_MAX;

        // Make the conbined dev/func parameter
        devfunc = dev;
        devfunc <<= 3;
        devfunc |= func;

        // Call the BIOS
        regs.w.ax = PCI_BIOS_CFG_WR32;
        regs.h.bh = bus;
        regs.h.bl = devfunc;
        regs.x.ecx = data;
        regs.w.di = reg;
        int386(PCI_INT, &regs, &regs);

        pciLastError = regs.h.ah;
        return pciLastError;
#else
		return 0;
#endif
}

//
//  PciSearchForDevice()
//
//  Input:  venid  - PCI vendor ID
//          devid  - PCI device ID
//          marker - Set to 0 on first call. will be incremented for every device found!
//
//  Return: pci_true if there are more devices to search for
//          pci_false if no more devices are present
//
//          bus    - bus of device found
//          dev    - dev of device found
//          func   - function of device found
//          marker - marker+1
//
UCHAR PciSearchForDevice(USHORT venid, USHORT devid, UCHAR* bus, UCHAR* dev, UCHAR* func, USHORT* marker)
{
#ifdef DOS_ENV
        union REGS regs;

        // Check parameters
        if(0==bus || 0==dev || 0==func || 0==marker)
        {
                return pci_false;
        }

        // Call the BIOS
        regs.w.ax = PCI_BIOS_FINDDEV;
        regs.w.dx = venid;
        regs.w.cx = devid;
        regs.w.si = *marker;
        int386(PCI_INT, &regs, &regs);

        // Device found?
        if(regs.h.ah==PCI_ERR_SUCCESS)
        {
                // fill in the return values
                *bus = regs.h.bh;
                *dev = regs.h.bl>>3;
                *func = regs.h.bl&0x7;

                // search for next device
                *marker += 1;

                return pci_true;
        }

        return pci_false;
#else
		return pci_true;
#endif
}
UINT DpmiMapPhysMemory(UINT start, UINT length)
{
#ifdef DOS_ENV
        union REGS regs;

        regs.w.ax = 0x0800;
        regs.w.bx = start>>16;
        regs.w.cx = start&0xffff;
        regs.w.si = length>>16;
        regs.w.di = length&0xffff;

        int386(0x31, &regs, &regs);

        if(regs.w.cflag&INTR_CF)
                return 0;

        return ((regs.w.bx<<16)&0xffff0000)|regs.w.cx;
#else
		return start;
#endif
}

//
//  DpmiUnMapPhysMemory()
//
//  Input:  mapmem - address returned by DpmiMapPhysMemory
//
//  Return: Nothing
//
void DpmiUnMapPhysMemory(UINT mapmem)
{
#ifdef DOS_ENV
        union REGS regs;

        regs.w.ax = 0x0801;
        regs.w.bx = mapmem>>16;
        regs.w.cx = mapmem&0xFFFF;

        int386(0x31, &regs, &regs);
#endif
}
#ifdef DOS_ENV
extern AHCI_EXT g_aAhciExt;
void Terminate()
{
    HwTerminate( &g_aAhciExt );
    SwTermiate( &g_aAhciExt );
    LogClose();
}
STATUS SwInit( PAHCI_EXT AhciExt )
{

    UINT address;
    UINT siz;
//    return SUCCESS;
    UINT nBar5;
    ASSERT( AhciExt != NULL );
    AhciExt->PciDevInfo.BusNum = 1;
    AhciExt->PciDevInfo.DevNum = 0;
    AhciExt->PciDevInfo.FuncNum = 0;
    nBar5 = PciReadConfigDWord( AhciExt->PciDevInfo.BusNum, AhciExt->PciDevInfo.DevNum, AhciExt->PciDevInfo.FuncNum, 0x24 );
    if ( ( nBar5 & 0x1 ) == 1 )
    {
        LogMsg( COMP_INIT, DBG_ERROR, "PCI MMI/O %08X isn't memory method!\n", nBar5);
        return UNSUCCESSFUL;
    }
    AhciExt->HBARegBaseAddress = (UINT)DpmiMapPhysMemory( nBar5, 0x1000 );
    printf("hba addr:0x%x\n",AhciExt->HBARegBaseAddress);
    if ( AhciExt->HBARegBaseAddress == (ULONG)NULL )
    {
        LogMsg( COMP_INIT, DBG_ERROR, "Failed to DpmiMapPhysicalMemory for CapabilityRegister!\n");
        return UNSUCCESSFUL;
    }
/*
Command List Base Address (CLB): Indicates the 32-bit base physical address for
the command list for this port. This base is used when fetching commands to execute.
The structure pointed to by this address range is 1K-bytes in length. This address must
be 1K-byte aligned as indicated by bits 09:00 being read only.
*/
    AhciExt->CLBRegAddress = (UINT*)( AhciExt->HBARegBaseAddress + 0x100 );

    *(volatile UINT *)AhciExt->CLBRegAddress = CMD_TABLE_HW_ADDRESS;
    printf("clb addr:0x%x\n",AhciExt->CLBRegAddress);

/*
FIS Base Address (FB):  Indicates the 32-bit base physical address for received
FISes. The structure pointed to by this address range is 256 bytes in length. This
address must be 256-byte aligned as indicated by bits 07:00 being read only. When
FIS-based switching is in use, this structure is 4KB in length and the address shall be
4KB aligned (refer to section  9.3.3).
*/
    AhciExt->FBRegAddress  = (UINT*)( AhciExt->HBARegBaseAddress + 0x100 + 0x08 );
/*
Commands Issued (CI):  This field is bit significant. Each bit corresponds to a
command slot, where bit 0 corresponds to command slot 0. This field is set by software
to indicate to the HBA that a command has been built in system memory for a
command slot and may be sent to the device. When the HBA receives a FIS which
clears the BSY, DRQ, and ERR bits for the command, it clears the corresponding bit in
this register for that command slot. Bits in this field shall only be set to ??by software
when PxCMD.ST is set to ??
This field is also cleared when PxCMD.ST is written from a ??to a ??by software.
*/
    AhciExt->PxCIRegAddress= (UINT*)( AhciExt->HBARegBaseAddress + 0x100 + 0x38 );

    printf("pxci addr:0x%x\n",AhciExt->PxCIRegAddress);
    LogMsg( COMP_INIT, DBG_NORMAL, "CLB  : %08X VAL: %08X \n", AhciExt->CLBRegAddress ,*(volatile UINT *)AhciExt->CLBRegAddress);
    LogMsg( COMP_INIT, DBG_NORMAL, "FB   : %08X\n", AhciExt->FBRegAddress );
    LogMsg( COMP_INIT, DBG_NORMAL, "PxCI : %08X\n", AhciExt->PxCIRegAddress );


    siz = DISK_A_SIZE + CMD_TABLE_SIZE;


    address = DpmiMapPhysMemory( CMD_TABLE_HW_ADDRESS, siz );
    if ( address == (UINT)NULL )
    {
        LogMsg( COMP_INIT, DBG_ERROR, "CT/DiskA: Map physical memory failed!\n");
        goto Error_exit;
    }else{
        AhciExt->HostCmdTable = (PHOST_CMD)address;
        AhciExt->DiskAAddrSw  = (UCHAR *)(address + CMD_TABLE_SIZE);
    }
    siz = DISK_B_SIZE + DISK_C_SIZE;
    address = DpmiMapPhysMemory( DISK_B_HW_ADDRESS, siz );
    if ( address == (UINT)NULL )
    {
        LogMsg( COMP_INIT, DBG_ERROR, "DiskB/C: Map physical memory failed!\n");
        goto Error_exit;
    }else{
        AhciExt->DiskBAddrSw = (UCHAR *)address;
        AhciExt->DiskCAddrSw = (UCHAR *)(address + DISK_B_SIZE);
    }
    printf("AhciExt->HostCmdTable:0x%x Phy:0x%x\n",AhciExt->HostCmdTable,CMD_TABLE_HW_ADDRESS);

    printf("AhciExt->DiskAAddrSw:0x%x Phy:0x%x\n",AhciExt->DiskAAddrSw,DISK_A_HW_ADDRESS);

    printf("AhciExt->DiskBAddrSw:0x%x Phy:0x%x\n",AhciExt->DiskBAddrSw,DISK_B_HW_ADDRESS);

    printf("AhciExt->DiskCAddrSw:0x%x Phy:0x%x\n",AhciExt->DiskCAddrSw,DISK_C_HW_ADDRESS);
    return SUCCESS;
Error_exit:
    if ( AhciExt->HostCmdTable != NULL )
    {
        DpmiUnMapPhysMemory( (UINT)AhciExt->HostCmdTable );
        AhciExt->HostCmdTable = NULL;
        AhciExt->DiskAAddrSw = (UCHAR *)NULL;
    }
    if ( AhciExt->DiskBAddrSw != (UCHAR *)NULL )
    {
        DpmiUnMapPhysMemory( AhciExt->DiskBAddrSw );
        AhciExt->DiskBAddrSw = (UCHAR *)NULL;
        AhciExt->DiskCAddrSw =(UCHAR *) NULL;
    }
    if ( AhciExt->HBARegBaseAddress != (ULONG)NULL )
    {
        DpmiUnMapPhysMemory( (UINT)AhciExt->HBARegBaseAddress );
        AhciExt->HBARegBaseAddress = (ULONG)NULL;
    }
    //AhciExt->CLBRegAddress = NULL;
    AhciExt->FBRegAddress  = NULL;
    AhciExt->PxCIRegAddress= NULL;
    return UNSUCCESSFUL;

}
STATUS SwTermiate( PAHCI_EXT AhciExt )
{
    if ( AhciExt->HostCmdTable != NULL )
    {
        DpmiUnMapPhysMemory( (UINT)AhciExt->HostCmdTable );
        AhciExt->HostCmdTable = NULL;
        AhciExt->DiskAAddrSw = (UCHAR *)NULL;
    }
    if ( AhciExt->DiskBAddrSw != (UCHAR *)NULL )
    {
        DpmiUnMapPhysMemory( AhciExt->DiskBAddrSw );
        AhciExt->DiskBAddrSw = (UCHAR *)NULL;
        AhciExt->DiskCAddrSw = (UCHAR *)NULL;
    }
	if ( AhciExt->HBARegBaseAddress != (ULONG)NULL )
	{
		DpmiUnMapPhysMemory( (UINT)AhciExt->HBARegBaseAddress );
		AhciExt->HBARegBaseAddress = (ULONG)NULL;
	}
    AhciExt->CLBRegAddress = NULL;
    AhciExt->FBRegAddress  = NULL;
    AhciExt->PxCIRegAddress= NULL;
    return SUCCESS;
}
STATUS SwInitDos()
{
    if ( SUCCESS != SwInit( &g_aAhciExt ) )
    {
        LogMsg( COMP_INIT, DBG_ERROR, "Failed to initialize software!\n");
        return UNSUCCESSFUL;
    }
    return SUCCESS;
}
#endif
