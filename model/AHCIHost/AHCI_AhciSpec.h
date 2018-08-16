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
*Filename     :  L0_AhciSpec.
*Version      :  Ver 1.0
*Date         :
*Author       :  Charles Zhou
*
*Description:
*Define registers and data structure for AHCI specification (v1.3).
*
*Modification History:
*2013/10/18   Charles Zhou 001 created for AHCI
*
*************************************************/
#ifndef _AHCI_SPEC_H
#define _AHCI_SPEC_H

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_HostInterface.h"

/*
 * Define AHCI Register
 *
 */

/* PxCMD */
#define POD_BIT         ( 1 << 2  )
#define HPCP_BIT        ( 1 << 18 )
#define SUD_BIT         ( 1 << 1 )
#define FR_BIT          ( 1 << 14 )
#define FRE_BIT         ( 1 << 4 )
#define ST_BIT          ( 1 )
#define CR_BIT          ( 1 << 15 )


#define AE_BIT          ( 1 << 31 )
#define IE_BIT          ( 1 << 1 )
#define HR_BIT          ( 1 )

#define DET_MASK        0xF

typedef union _AHCI_PORT_IS_REG {

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

}  AHCI_PORT_IS_REG, *PAHCI_PORT_IS_REG;

/***************************************************************************\
 *
 * FIS definition
 *
\***************************************************************************/

/*
   Figure 272 C FIS type value assignments

   27h Register Host to Device FIS
   34h Register Device to Host FIS
   39h DMA Activate FIS C Device to Host
   41h DMA Setup FIS C Bi-directional
   46h Data FIS C Bi-directional
   58h BIST Activate FIS C Bi-directional
   5Fh PIO Setup FIS C Device to Host
   A1h Set Device Bits FIS C Device to Host
   A6h Reserved for future Serial ATA definition
   B8h Reserved for future Serial ATA definition
   BFh Reserved for future Serial ATA definition
   C7h Vendor specific
   D4h Vendor specific
   D9h Reserved for future Serial ATA definition
*/

#define RH2D_FIS_TYPE                   0x27
#define RD2H_FIS_TYPE                   0x34
#define DMA_SETUP_FIS_TYPE              0x41
#define PIO_SETUP_FIS_TYPE              0x5F
#define SET_DEVICE_BITS_FIS_TYPE        0xA1
/*
 *     1.  20h (Read Sectors,         PIO Data-In 0D, 71, E3)
 *     2.  24h (Read Sectors Ext,     PIO Data-In)
 *     3.  25h (Read DMA Ext,         DMA )
 *     4.  29h (Read Multipile Ext,   PIO Data-IN)
 *     5.  2Fh (Read Log Ext,         PIO Data-IN)
 *     6.  3Dh (Write DMA FUA Ext,    DMA )
 *     7.  34h (Write Sectors Ext,    PIO Data-Out )
 *     8.  60h (Read FPDMA Queued,    DMA Queued (NCQ) )
 *     9.  91h
 *     10. B0h (Smart Disable Operation )
 *             DA, Non-Data
 *     11. C4h (Read Multiple,        PIO Data-In)
 *             71h, B5, E3h
 *     12. C6h (Set Multiple Mode,    Non-Data, E3)
 *     13. C8h (Read DMA,             DMA, E3)
 *     14. E4h (Read Buffer,          PIO Data-In )
 *     15. E7h (Flush Cache,          Non-Data )
 *     16. E8h (Write Buffer,         PIO Data-Out )
 *     17. ECh (Identify Device,      PIO Data-In )
 *     18. EFh (Set Feature,          Non-Data )
 *     19. F5h (Security Freeze Lock, Non-Data )
 */

//#define ATA_READ_SECTORS        0x20
//#define ATA_READ_SECTORS_EXT    0x24
//#define ATA_READ_DMA_EXT        0x25
//#define ATA_IDENTIFY_DEVICE     0xEC




typedef struct _cfis{
        U8      Type;
        U8      PMPort:4;
        U8      Rsvd:3;
        U8      C:1;
        U8      Command;
        U8      FeatureLo;
        U32     LBALo:24;
        U32     Device:8;
        U32     LBAHi:24;
        U32     FeatureHi:8;
        U16     Count;
        U8      ICC;
        U8      Control;
        U32     Auxiliary;
}CFIS, *PCFIS;

typedef struct _rfis
{
        U8      Type;
        U8      PMPort:4;
        U8      Rsvd1:2;
        U8      I:1;
        U8      Rsvd2:1;
        U8      Status;
        U8      Error;
        U32     LBALo:24;
        U32     Device:8;
        U32     LBAHi:24;
        U32     Rsvd3:8;
        U16     Count;
        U16     Rsvd4;
        U32     Rsvd5;
}RD2HFIS, *PRD2HFIS;

typedef struct _sdbfis
{
        U8      Type;
        U8      PMPort:4;
        U8      Rsvd1:2;
        U8      I:1;
        U8      Rsvd2:1;
        union{
            struct{
                U8      StatusLo:4;
                U8      StatusHi:4;
            };
            U8 Status;
        };
        U8      Error;
        U32     ACT;
}SDBFIS, *PSDBFIS;

typedef struct _dsfis
{
        U8      Type;   // 0x41
        U8      PMPort:4;
        U8      Rsvd1:1;
        U8      D:1;
        U8      I:1;
        U8      A:1;
        U16     Rsvd2;
        U32     DMABufferIDLo;
        U32     DMABufferIDHi;
        U32     Rsvd3;
        U32     DMABufferOffset;
        U32     DMATransferCount;
        U32     Rsvd4;
}DSFIS, *PDSFIS;


typedef struct _psfis
{
        U8      Type;
        U8      PMPort:4;
        U8      Rsvd1:1;
        U8      D:1;
        U8      I:1;
        U8      Rsvd2:1;
        U8      Status;
        U8      Error;
        U32     LBALo:24;
        U32     Device:8;
        U32     LBAHi:24;
        U32     Rsvd3:8;
        U16     Count;
        U8      Rsvd4;
        U8      E_Status;
        U16     TransferCount;
        U16     Rsvd5;
}PSFIS, *PPSFIS;


/*
 * For AHCI spec.
 */
#define PSFIS_OFFSET    0x20
#define RD2HFIS_OFFSET  0x40
#define DSFIS_OFFSET    0
#define SDBFIS_OFFSET   0x58

typedef struct _receivefis{
    DSFIS       DSFis;
    U32         Rsvd1;
    PSFIS       PSFis;
    U32         Rsvd2[ 3 ];
    RD2HFIS     RD2HFis;
    U32         Rsvd13;
    SDBFIS      SDBFis;
}RFIS, *PRFIS;



typedef struct _command_header{
        U8      CFL:5;
        U8      A:1; // ATAPI
        U8      W:1; // Write
        U8      P:1; // Prefetch

        U8      R:1; // Reset
        U8      B:1; // BIST
        U8      C:1; // Clear Busy upon R_OK
        U8      Rsvd1:1;
        U8      PMPort:4;

        U16     PRDTL;

        U32     PRDBC;
        U32     CTBALo;
        U32     CTBAHi;
        U32     Rsvd2[ 4 ];
}COMMAND_HEADER, *PCOMMAND_HEADER;

#define HOST_MAX_PRDT_NUM   (MAX_PRDT_NUM)
typedef struct _command_table{
    union{
    CFIS    CFis;
    U32     Rsvd1[ 16 ];
    };
    U32     ACMD[ 4 ];
    U32     Rsvd2[ 12 ];
    PRD     Prdt[ HOST_MAX_PRDT_NUM ];
}COMMAND_TABLE, *PCOMMAND_TABLE;

/*
 * AHCI Register Definition
 */
typedef union _px_tfd{
        struct {
            union{
                struct{
                    U8 ERR:1;
                    U8 CS2_1:2;
                    U8 DRQ:1;
                    U8 CS6_4:3;
                    U8 BSY:1;
                };
                U8 Status;
            };
            U8 Error;
            U16 Rsvd;
        };
        U32 AsU32;
}PxTFD, *PPxTFD;

typedef enum _ATACMD_STATUS
{
    ATASTATUS_SUCCESS = 0,
    ATASTATUS_CLEARBUSY,
    ATASTATUS_ABORT,
    ATASTATUS_RPTNCQERR,
    ATASTATUS_CLRNCQERR
} ATACMD_STATUS;

/*
 * calculate LBA address according to CFIS pointer
 */
#define LBA_ADDRESS(_x_) ( ( (_x_)->LBAHi * 0x1000000 ) + (_x_)->LBALo )

/*
 * Advance PrdIndex and PrdOffset of Prdt according to Length.
 */
U32 L0_AhciAdvancePrdLocation(PPRD Prdt, U16* PrdIndex, U32* PrdOffset, U32 Length );
/*------------------------------------------------------------------------------
Name: HAL_GetLocalPrdEntryAddr
Description:
    get local-saved(in SRAM) PRD entry address.
Input Param:
    U8 CmdTag: tag of host comamnd
    U8 Index: PRD index in PRDT table
Output Param:
    none
Return Value:
    U32: address of PRD entry
Usage:
    befroe L1/L2/L3 read PRD entry information, FW call this interface to get its address
History:
    20131014    Gavin   created
------------------------------------------------------------------------------*/

//NOTE: L0 is in charge of PRDT storage, so this interface shoud be written by L0 designer
U32 L0_AhciGetLocalPrdEntryAddr(U8 CmdTag, U16 Index);

#endif // _AHCI_SPEC_H
