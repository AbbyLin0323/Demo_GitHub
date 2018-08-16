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

******************************************************************************/

#ifndef _H_AHCI_HOSTMODEL
#define _H_AHCI_HOSTMODEL
#include "windows.h"
#include "checklist_parse.h"

#define MAX_CFIS_DW    (5)   //5DW

#define KB                          (1024)
#define DATA_BUFFER_PER_CMD         (4096 * KB)

// define Port status
#define AHCI_PORT_WAIT_INI  0x1
#define AHCI_PORT_WAIT_FR   (AHCI_PORT_WAIT_INI + 1)
#define AHCI_PORT_WAIT_CR   (AHCI_PORT_WAIT_FR + 1)
#define AHCI_PORT_ERROR_WAIT_RESET (AHCI_PORT_WAIT_CR + 1)
#define AHCI_PORT_ERROR_WAIT_CMDSTOP (AHCI_PORT_ERROR_WAIT_RESET + 1)
#define AHCI_PORT_ERROR_RESTART (AHCI_PORT_ERROR_WAIT_CMDSTOP + 1)
#define AHCI_PORT_START     (AHCI_PORT_ERROR_RESTART + 1)
#define AHCI_LARGE_BUFFER_SIZE  64*1024*1024

//typedef U32 (*GetSecCntFunc)(PAHCI_H2D_REGISTER_FIS);

typedef struct _LARGE_BUFFER_MANAGEMENT
{
    U8 *pLargeDataBuffer;
    BOOL bUsed;
}LARGE_BUFFER_MANAGEMENT;

typedef union _AHCI_COMMAND {

    struct {
        //LSB
        ULONG ST :1; // Start: When set, the HBA may process the command list. When cleared, the HBA may not process the command list. Whenever this bit is changed from a ?0 to a, the HBA starts processing the command list at entry ?0 . Whenever this bit is changed from a ?1 to a ?0 , the PxCI register is cleared by the HBA upon the HBA putting the controller into an idle state. This bit shall only be set to ??by software after PxCMD.FRE has been set to ?? Refer to section 10.3.1 for important restrictions on when ST can be set to ?1 .
        ULONG SUD :1; // Spin-Up Device: This bit is read/write for HBAs that support staggered spin-up via CAP.SSS. This bit is read only ?1 for HBAs that do not support staggered spin-up. On an edge detect from ?0 to ?1 , the HBA shall start a COMRESET initializatoin sequence to the device. Clearing this bit to ??does not cause any OOB signal to be sent on the interface.  When this bit is cleared to ??and PxSCTL.DET=0h, the HBA will enter listen mode as detailed in section 10.9.1.
        ULONG POD :1; // Power On Device: This bit is read/write for HBAs that support cold presence detection on this port as indicated by PxCMD.CPD set to ?1 . This bit is read only ?1 for HBAs that do not support cold presence detect. When set, the HBA sets the state of a pin on the HBA to ?1 so that it may be used to provide power to a cold-presence detectable port.
        ULONG CLO :1; // Command List Override: Setting this bit to ?1 causes PxTFD.STS.BSY and PxTFD.STS.DRQ to be cleared to ?0 . This allows a software reset to be transmitted to the device regardless of whether the BSY and DRQ bits are still set in the PxTFD.STS register. The HBA sets this bit to ?0 when PxTFD.STS.BSY and PxTFD.STS.DRQ have been cleared to ?0 . A write to this register with a value of ?0 shall have no effect. This bit shall only be set to ?1 immediately prior to setting the PxCMD.ST bit to ?1 from previous value of ?0 . Setting this bit to ?1 at any other time is not supported and will result in indeterminate behavior.
        ULONG FRE :1; // FIS Receive Enable: When set, the HBA may post received FISes into the FIS receive area pointed to by PxFB (and for 64-bit HBAs, PxFBU). When cleared, received FISes are not accepted by the HBA, except for the first D2H register FIS after the initialization sequence, and no FISes are posted to the FIS receive area. System software must not set this bit until PxFB (PxFBU) have been programmed with valid pointer to the FIS receive area, and if software wishes to move the base, this bit must first be cleared, and software must wait for the FR bit in this register to be cleared. Refer to section 10.3.2 for important restrictions on when FRE can be set and cleared.
        ULONG DW6_Reserved :3; //

        ULONG CCS :5; // Current Command Slot: This field is valid when P0CMD.ST is set to ?1 and shall be set to the command slot value of the command that is currently being issued by the HBA. When P0CMD.ST transitions from ?1 to ?0 , this field shall be reset to ?0 . After P0CMD.ST transitions from ?0 to ?1 , the highest priority slot to issue from next is command slot 0. After the first command has been issued, the highest priority slot to issue from next is P0CMD.CCS + 1. For example, after the HBA has issued its first command, if CCS = 0h and P0CI is set to 3h, the next command that will be issued is from command slot 1.
        ULONG MPSS :1; // Mechanical Presence Switch State: The MPSS bit reports the state of an interlocked switch attached to this port. If CAP.SIS is set to ?1? and the interlocked switch is closed then this bit is cleared to ?0 . If CAP.SIS is set to ?1 and the interlocked switch is open then this bit is set to ?1 . If CAP.SIS is set to ?0 then this bit is cleared to ?0 . Software should only use this bit if both CAP.SIS and P0CMD.ISP are set to ?1 .
        ULONG FR :1; // FIS Receive Running: When set, the FIS Receive DMA engine for the port is running. See section 10.3.2 for details on when this bit is set and cleared by the HBA.
        ULONG CR :1; // Command List Running: When this bit is set, the command list DMA engine for the port is running. See the AHCI state machine in section 5.2.2 for details on when this bit is set and cleared by the HBA.

        ULONG CPS :1; // Cold Presence State: The CPS bit reports whether a device is currently detected on this port. If CPS is set to ?1 , then the HBA detects via cold presence that a device is attached to this port. If CPS is cleared to ?0 , then the HBA detects via cold presence that there is no device attached to this port.
        ULONG PMA :1; // Port Multiplier Attached: This bit is read/write for HBAs that support a Port Multiplier (CAP.SPM = ?1 ). This bit is read-only for HBAs that do not support a port Multiplier (CAP.SPM = ?0 ). When set to ?1 by software, a Port Multiplier is attached to the HBA for this port. When cleared to ?0 by software, a Port Multiplier is not attached to the HBA for this port. Software is responsible for detecting whether a Port Multiplier is present; hardware does not auto-detect the presence of a Port Multiplier.
        ULONG HPCP :1; // Hot Plug Capable Port:  When set to ?? indicates that this port's signal and power connectors are externally accessible via a joint signal and power connector for blindmate device hot plug.  When cleared to ?? indicates that this port's signal and power connectors are not externally accessible via a joint signal and power connector.
        ULONG MPSP :1; // Mechanical Presence Switch Attached to Port : If set to ?1 , the platform supports an interlocked switch attached to this port. If cleared to ?0 , the platform does not support an interlocked switch attached to this port. When this bit is set to ?1 , P0CMD.HPCP should also be set to ?1 .
        ULONG CPD :1; // Cold Presence Detection: If set to ?1 , the platform supports cold presence detection on this port. If cleared to ?0 , the platform does not support cold presence detection on this port. When this bit is set to ?1 , P0CMD.HPCP should also be set to ?1 .
        ULONG ESP :1; //AHCI 1.1 External SATA Port: When set to '1', indicates that this port's signal connector is externally accessible on a signal only connector. When set to '1', CAP.SXS shall be set to '1'. When cleared to ?? indicates that this port's signal connector is not externally accessible on a signal only connector.  ESP is mutually exclusive with the HPCP bit in this register.
        ULONG FBSCP :1; // FIS-based Switching Capable Port (FBSCP): When set to ?? indicates that this port supports Port Multiplier FIS-based switching.  When cleared to ?? indicates that this port does not support FIS-based switching. This bit may only be set to ??if both CAP.SPM and CAP.FBSS are set to ??
        ULONG APSTE :1; // Automatic Partial to Slumber Transitions Enabled (APSTE): When set to ?? the HBA may perform Automatic Partial to Slumber Transitions.  When cleared to ??the port shall not perform Automatic Partial to Slumber Transitions.  Software shall only set this bit to ??if CAP2.APST is set to ?? if CAP2.APST is cleared to ??software shall treat this bit as reserved.

        ULONG ATAPI :1; // Device is ATAPI: When set, the connected device is an ATAPI device. This bit is used by the HBA to control whether or not to generate the desktop LED when commands are active. See section 10.10 for details on the activity LED.
        ULONG DLAE :1; // Drive LED on ATAPI Enable: When set, the HBA shall drive the LED pin active for commands regardless of the state of P0CMD.ATAPI. When cleared, the HBA shall only drive the LED pin active for commands if P0CMD.ATAPI set to ?0 . See section 10.10 for details on the activity LED.
        ULONG ALPE :1; // Aggressive Link Power Management Enable: When set to '1', the HBA shall aggressively enter a lower link power state (Partial or Slumber) based upon the setting of the ASP bit. Software shall only set this bit to ??if CAP.SALP is set to ?? if CAP.SALP is cleared to ??software shall treat this bit as reserved. See section 8.3.1.3 for details.
        ULONG ASP :1; // Aggressive Slumber / Partial: When set to '1', and ALPE is set, the HBA shall aggressively enter the Slumber state when it clears the PxCI register and the PxSACT register is cleared or when it clears the PxSACT register and PxCI is cleared. When cleared, and ALPE is set, the HBA shall aggressively enter the Partial state when it clears the PxCI register and the PxSACT register is cleared or when it clears the PxSACT register and PxCI is cleared.  If CAP.SALP is cleared to ??software shall treat this bit as reserved.  See section 8.3.1.3 for details.
        ULONG ICC :4; // Interface Communication Control: This field is used to control power management states of the interface. If the Link layer is currently in the L_IDLE state, writes to this field shall cause the HBA to initiate a transition to the interface power management state requested. If the Link layer is not currently in the L_IDLE state, writes to this field shall have no effect.
        //MSB
    };

    ULONG AsUlong;

}  AHCI_COMMAND, *PAHCI_COMMAND;


void AHCI_HostWriteToOTFB(
                            U32 HostAddrHigh,
                            U32 HostAddrLow,
                            U32 OTFBAddr,
                            U32 ByteLen,
                            U8 nTag);

void AHCI_HostWriteToDram(
                            U32 HostAddrHigh,
                            U32 HostAddrLow,
                            U32 DramAddr,
                            U32 ByteLen,
                            U8 nTag);

void AHCI_HostReadFromOTFB(
                            U32 HostAddrHigh,
                            U32 HostAddrLow,
                            U32 OTFBAddr,
                            U32 ByteLen,
                            U8 nTag);

void AHCI_HostReadFromDram(
                            U32 HostAddrHigh,
                            U32 HostAddrLow,
                            U32 DramAddr,
                            U32 ByteLen,
                            U8 nTag);

BOOL AHCI_InitHBA( void );
BOOL AHCI_IsHBARegActive(void);
void AHCI_HCmdToRowCmd(HCMD_INFO* pHcmd);
void AHCI_HostParseRFis(U8 ucCmdTag,U32* pFeedBack);

#endif