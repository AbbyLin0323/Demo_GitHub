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
Filename     :   AHCI_HostModel.h                                         
Version      :   0.1                                              
Date         :   2013.08.26                                         
Author       :   bettywu

Description:  the interface to other model
Others: 
Modification History:
20130826 create

*******************************************************************/

#ifndef _H_AHCI_HOSTMODELVAR
#define _H_AHCI_HOSTMODELVAR

#include <windows.h>
#include "HAL_HCT.h"
#include "HAL_HostInterface.h"
#include "SimATA_Interface.h"
#include "AHCI_AhciSpec.h"

//#include "L0_Interface.h"

#define SEC_BITS_PER_PRD    6  
#define SEC_PER_PRD         (1<<SEC_BITS_PER_PRD)   // sector count,64
#define SIZE_PER_PRD        (SEC_BITS_PER_PRD <<9)  // 32K

#define RANDOM_SEC_ARRAY_SIZE  80

#define PRD_DATA_BYTE_MAX   (1 << 22)

extern CRITICAL_SECTION g_CmdCriticalSection;

typedef unsigned __int64 U64;
typedef SATA_H2D_REGISTER_FIS AHCI_H2D_REGISTER_FIS ;

typedef struct _AHCI_PORT {

    //Port Command List Base Address as defined in AHCI1.0 section 3.3.1
    U32 CLB;
    //Command List Base Address Upper 32-bits as defined in AHCI1.0 section 3.3.2
    U32 CLBU; //Command List Base Address Upper: Indicates the upper 32-bits for the command list base physical address for this port. This base is used when fetching commands to execute. This register shall be read only ?0 for HBAs that do not support 64-bit addressing.

    //FIS Base Address as defined in AHCI1.0 section 3.3.3
    U32 FB;
    //FIS Base Address Upper 32-bits as defined in AHCI section 3.3.4
    U32 FBU; //FIS Base Address Upper: Indicates the upper 32-bits for the received FIS base physical address for this port. This register shall be read only ?0 for HBAs that do not support 64-bit addressing.

    U32 IS;

    U32 IE;

    U32  CMD;

    U32 DW7_Reserved;

    U32 TFD;

    U32 SIG;

    U32 SSTS;

    U32 SCTL;

    U32 SERR;

    //Serial ATA Active as defined in AHCI1.0 section 3.3.13
    U32 SACT; //Device Status: Device Status (DS):  This field is bit significant.  Each bit corresponds to the TAG and command slot of a native queued command, where bit 0 corresponds to TAG 0 and command slot 0. This field is set by software prior to issuing a native queued command for a particular command slot.  Prior to writing PxCI[TAG] to ?? software will set DS[TAG] to ??to indicate that a command with that TAG is outstanding.  The device clears bits in this field by sending a Set Device Bits FIS to the host.  The HBA clears bits in this field that are set to ??in the SActive field of the Set Device Bits FIS.  The HBA only clears bits that correspond to native queued commands that have completed successfully.  Software should only write this field when PxCMD.ST is set to ??  This field is cleared when PxCMD.ST is written from a ??to a ??by software.  This field is not cleared by a COMRESET or a software reset.

    //Command Issue    as defined in AHCI1.0 section 3.3.14
    U32 CI; //Commands Issued: This field is bit significant.  Each bit corresponds to a command slot, where bit 0 corresponds to command slot 0.  This field is set by software to indicate to the HBA that a command has been built in system memory for a command slot and may be sent to the device.  When the HBA receives a FIS which clears the BSY, DRQ, and ERR bits for the command, it clears the corresponding bit in this register for that command slot. Bits in this field shall only be set to ??by software when PxCMD.ST is set to ?? This field is also cleared when PxCMD.ST is written from a ??to a ??by software.

    U32 SNTF;

    U32 ReservedForFIS_Based_Switching;

    U32 DEVSLP;

    U32 Reserved[10];

    U32 VendorSpecific[4];
}  AHCI_PORT, *PAHCI_PORT;




typedef struct _PRD_ENTRY
{
    //UINT32 Reserved1:1;
    U32 DBA;          // data base addr
    U32 DBAU;            // data base addr upper
    U32 Reserved32;
    U32 DBC:22;          // Data byte count 
    U32 Reserved9:9;
    U32 IntOnCmp:1;      // interrupte on completion

}PRD_ENTRY;

typedef struct _CMD_TABLE
{
    AHCI_H2D_REGISTER_FIS CmdFis;
    U32 ACMD[4];
    U64 nTransferLba;
    U32 nTransferByte;
    U32 ulStartLba;
    U32 ulLbaLen;
    U32 Rsved[7];
    PRD_ENTRY PrdTable[HOST_MAX_PRDT_NUM];
    U8 *pDataBuffer;

}CMD_TABLE;


typedef struct _CMD_HEADER
{
    //dw0
    U32 CmdFisLen:5;
    U32 ATAPIFlag:1;
    U32 WriteFlag:1;
    U32 PrefetchFlag:1;
    U32 ResetFlag:1;
    U32 BISTFisFlag:1;
    U32 BusyFlag:1;
    U32 Reserved1:1;
    U32 MultiplierPort:4;
    U32 PrdTLen:16;

    //dw1
    U32 PrdByteCnt;

    //dw2
    //U32 Reserved7:7;
    U32 CTBaseAddr;

    //dw3
    U32 CTBaseAddrU:32;

    //dw4
    U32 ResvDW[4];

}CMD_HEADER;

typedef struct _wbq_mgr{
    CRITICAL_SECTION    WBQCriticalSection;
    HANDLE              EventTable[ 4 ];    // 0: for command state trigger event
                                            // 1: for data transfer done event.
                                            // 2: for thread exit event.
    U8                  ActiveWBQBitmap[MAX_SLOT_NUM];    // each bit indicates the corresponding command has been in WBQ trigger state.
    U8                  DataDoneBitmap[MAX_SLOT_NUM];     // each bit indicates the corresponding command has completed data transfer.
    U8                  bsWBQOffset[ MAX_SLOT_NUM ];
    
}HCT_MGR, *PHCT_MGR;

typedef union _AHCI_PORT_IS_REG_MODEL {

    struct {
        //LSB
        U32 DHRS:1; //Device to Host Register FIS Interrupt: A D2H Register FIS has been received with the 'I' bit set, and has been copied into system memory.
        U32 PSS :1; //PIO Setup FIS Interrupt: A PIO Setup FIS has been received with the 'I' bit set, it has been copied into system memory, and the data related to that FIS has been transferred. This bit shall be set even if the data transfer resulted in an error.
        U32 DSS :1; //DMA Setup FIS Interrupt: A DMA Setup FIS has been received with the 'I' bit set and has been copied into system memory.
        U32 SDBS :1; //Set Device Bits Interrupt: A Set Device Bits FIS has been received with the 'I' bit set and has been copied into system memory.
        U32 UFS :1; //Unknown FIS Interrupt: When set to '1 , indicates that an unknown FIS was received and has been copied into system memory. This bit is cleared to ?0 by software clearing the PxSERR.DIAG.F bit to ?0 . Note that this bit does not directly reflect the PxSERR.DIAG.F bit. PxSERR.DIAG.F is set immediately when an unknown FIS is detected, whereas this bit is set when that FIS is posted to memory. Software should wait to act on an unknown FIS until this bit is set to ?1 or the two bits may become out of sync.
        U32 DPS :1; //Descriptor Processed: A PRD with the 'I' bit set has transferred all of its data. Refer to section 5.3.2.
        U32 PCS :1; //Port Connect Change Status: 1=Change in Current Connect Status. 0=No change in Current Connect Status. This bit reflects the state of PxSERR.DIAG.X. This bit is only cleared when PxSERR.DIAG.X is cleared.
        U32 DMPS :1; //Device Mechanical Presence Status (DMPS): When set, indicates that a mechanical presence switch attached to this port has been opened or closed, which may lead to a change in the connection state of the device.  This bit is only valid if both CAP.SMPS and P0CMD.MPSP are set to ??
        U32 DW4_Reserved :14;
        U32 PRCS :1; //PhyRdy Change Status: When set to '1 indicates the internal PhyRdy signal changed state. This bit reflects the state of P0SERR.DIAG.N. To clear this bit, software must clear P0SERR.DIAG.N to ?0 .
        U32 IPMS :1; //Incorrect Port Multiplier Status: Indicates that the HBA received a FIS from a device whose Port Multiplier field did not match what was expected.  The IPMS bit may be set during enumeration of devices on a Port Multiplier due to the normal Port Multiplier enumeration process.  It is recommended that IPMS only be used after enumeration is complete on the Port Multiplier.
        U32 OFS :1; //Overflow Status: Indicates that the HBA received more bytes from a device than was specified in the PRD table for the command.
        U32 DW4_Reserved2 :1;
        U32 INFS :1; //Interface Non-fatal Error Status: Indicates that the HBA encountered an error on the Serial ATA interface but was able to continue operation. Refer to section 6.1.2.
        U32 IFS :1; //Interface Fatal Error Status: Indicates that the HBA encountered an error on the Serial ATA interface which caused the transfer to stop. Refer to section 6.1.2.
        U32 HBDS :1; //Host Bus Data Error Status: Indicates that the HBA encountered a data error (uncorrectable ECC / parity) when reading from or writing to system memory.
        U32 HBFS :1; //Host Bus Fatal Error Status: Indicates that the HBA encountered a host bus error that it cannot recover from, such as a bad software pointer. In PCI, such an indication would be a target or master abort.
        U32 TFES :1; //Task File Error Status: This bit is set whenever the status register is updated by the device and the error bit (bit 0) is set.
        U32 CPDS :1; //Cold Port Detect Status: When set, a device status has changed as detected by the cold presence detect logic. This bit can either be set due to a non-connected port receiving a device, or a connected port having its device removed. This bit is only valid if the port supports cold presence detect as indicated by PxCMD.CPD set to ?1 .
        //MSB
    };

    U32 AsUlong;

}  AHCI_PORT_IS_REG_MODEL, *PAHCI_PORT_IS_REG_MODEL;
// AHCI model global var
extern volatile HCT_FCQ_REG *g_pFCQReg;//(PHCT_FCQ)&rHCT_FCQ_REG;
extern volatile HCT_WBQ_REG *g_pWBQReg;
extern volatile AHCI_PORT *g_pPortHBAReg;

extern CRITICAL_SECTION g_csCSTCriticalSection;
extern CRITICAL_SECTION g_csFCQCriticalSection;
extern CRITICAL_SECTION g_csRegCriticalSection;

extern U8 g_uWBQWritePointer[MAX_SLOT_NUM];
extern U8 g_uWBQReadPointer[MAX_SLOT_NUM];

extern U8 g_uFCQReadPointer;
extern U8 g_uFCQWritePointer;
extern U8 g_uFCQEmptyCnt;

#endif