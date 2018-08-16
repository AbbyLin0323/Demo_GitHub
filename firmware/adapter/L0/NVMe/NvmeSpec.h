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
* File Name    : NvmeSpec.h
* Discription  : 
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef __NVMESPEC_H__
#define __NVMESPEC_H__

/* MEMORY STRUCTURES */

#include "BaseDef.h"
#define NUM_LBAF (16)

typedef U8              UCHAR;
typedef U16             USHORT;
typedef unsigned long             ULONG;
typedef unsigned long long             ULONGLONG;



/* Get Log Page - Log Identifiers, Section 5.10.1, Figure 57 */
#define ERROR_INFORMATION           0x01
#define SMART_HEALTH_INFORMATION    0x02
#define FIRMWARE_SLOT_INFORMATION   0x03

/*
 * Get Log Page - Error Information Log Entry
 *
 * Section 5.10.1.1, Figure 58, (Log Identifier 0x01)
 */
typedef struct _ADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY
{
    /*
     * This is a 64-bit incrementing error count, indicating a unique identifier
     * for this error. The error count starts at 1h, is incremented for each
     * unique error log entry, and is retained across power off conditions. A
     * value of 0h indicates an invalid entry; this value may be used when there
     * are lost entries or when there a fewer errors than the maximum number of
     * entries the controller supports.
     */
    struct
    {
        ULONG DW0;
        ULONG DW1;
    }ErrorCount;

    /*
     * This field indicates the Submission Queue Identifier of the command that
     * the error information is associated with.
     */
    USHORT      SubmissionQueueID;

    /*
     * This field indicates the Command Identifier of the command that the error
     * is assocated with.
     */
    USHORT      CommandID;

    /* This field indicates the Status that the command completed with. */
    struct
    {
        /* Phase Tag posted for the command */
        USHORT  PhaseTag   :1;
        /* The reported status for the completed command. */
        USHORT  Status     :15;
    } StatusField;
    /*
     * This field indicates the byte and bit of the command parameter that the
     * error is associated with, if applicable. If the parameter spans multiple
     * bytes or bits, then the location indicates the first byte and bit of the
     * parameter.
     */
    union
    {
        struct
        {
            /* Byte in command that contained the error. Valid values are 0 to 63 */
            USHORT  ByteInCommand   :8;

            /* Bit in command that contained the error. Valid values are 0 to7. */
            USHORT  BitInCommand    :3;
            USHORT  Reserved        :5;
        };

        U16 usValue;
    } ParameterErrorLocation;

    struct
    {
        ULONG DW0;
        ULONG DW1;
    }LBA;
    
    ULONG       Namespace;
    UCHAR       VendorSpecificInformationAvailable;
    UCHAR       Reserved[35];
} ADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY,
  *PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY;


/* Remove default compiler padding for SMART log page */
#pragma pack(push, smart_log, 1)

/*
 * Get Log Page - SMART/Health Information Log
 *
 * Section 5.10.1.2, Figure 59, (Log Identifier 0x02)
 */
typedef struct _NVM_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY
{
    /*
     * This field indicates critical warnings for the state of the controller.
     * Each bit corresponds to a critical warning type; multiple bits may be
     * set. If a bit is cleared to '0' then that critical warning does not
     * apply. Critical warnings may result in an asynchronous event notification
     * to the host.
     */
    struct
    {

        /*
         * If set to '1' then the available spare space has fallen below the
         * threshold.
         */
        UCHAR   AvailableSpareSpaceBelowThreshold            :1;

        /*
         * If set to '1' then the temperature has exceeded a critical
         * threshold.
         */
        UCHAR   TemperatureExceededCriticalThreshold    :1;

        /*
         * If set to '1' then the device reliability has been degraded due to
         * significant media related errors or any internal error that degrades
         * device reliability.
         */
        UCHAR   DeviceReliablityDegraded                :1;

        /* If set to '1' then the media has been placed in read only mode. */
        UCHAR   MediaInReadOnlyMode                     :1;

        /*
         * If set to '1' then the volatile memory backup device has failed.
         * This field is only valid if the controller has a volatile memory
         * backup solution.
         */
        UCHAR   VolatileMemoryBackupDeviceFailed        :1;
        UCHAR   Reserved                                :3;
    } CriticalWarning;

    /*
     * Contains the temperature of the overall device (controller and NVM
     * included) in units of Kelvin. If the temperature exceeds the temperature
     * threshold, refer to section 5.12.1.4, then an asynchronous event may be
     * issued to the host.
     */
    USHORT      Temperature;

    /*
     * Contains a normalized percentage (0 to 100%) of the remaining spare
     * capacity available.
     */
    UCHAR       AvailableSpare;

    /*
     * When the Available Spare falls below the threshold indicated in this
     * field, an asynchronous event may be issued to the host. The value is
     * indicated as a normalized percentage (0 to 100%).
     */
    UCHAR       AvailableSpareThreshold;

    /*
     * Contains a vendor specific estimate of the percentage of device life used
     * based on the actual device usage and the manufacturer’s prediction of
     * device life. A value of 100 indicates that the estimated endurance of the
     * device has been consumed, but may not indicate a device failure. The
     * value is allowed to exceed 100. Percentages greater than 254 shall be
     * represented as 255. This value shall be updated once per power-on hour
     * (when the controller is not in a sleep state). Refer to the JEDEC JESD218
     * standard for SSD device life and endurance measurement techniques.
     */
    UCHAR       PercentageUsed;
    UCHAR       Reserved1[26];

    /*
     * Contains the number of 512 byte data units the host has read from the
     * controller; this value does not include metadata. This value is reported
     * in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes
     * read) and is rounded up. When the LBA size is a value other than 512
     * bytes, the controller shall convert the amount of data read to 512 byte
     * units. For the NVM command set, logical
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } DataUnitsRead;

    /*
     * Contains the number of 512 byte data units the host has written to the
     * controller; this value does not include metadata. This value is reported
     * in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes
     * written) and is rounded up. When the LBA size is a value other than 512
     * bytes, the controller shall convert the amount of data written to 512
     * byte units. For the NVM command set, logical blocks written as part of
     * Write operations shall be included in this value. Write Uncorrectable
     * commands shall not impact this value.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } DataUnitsWritten;

    /*
     * Contains the number of read commands issued to the controller. For the
     * NVM command set, this is the number of Compare and Read commands.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } HostReadCommands;

    /*
     * Contains the number of write commands issued to the controller. For the
     * NVM command set, this is the number of Write commands.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } HostWriteCommands;

    /*
     * Contains the amount of time the controller is busy with I/O commands. The
     * controller is busy when there is a command outstanding to an I/O Queue
     * (specifically, a command was issued via an I/O Submission Queue Tail
     * doorbell write and the corresponding completion entry has not been posted
     * yet to the associated I/O Completion Queue). This value is reported in
     * minutes.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } ControllerBusyTime;

    /* Contains the number of power cycles. */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } PowerCycles;

    /*
     * Contains the number of power-on hours. This does not include time that
     * the controller was powered and in a low power state condition.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } PowerOnHours;

    /*
     * Contains the number of unsafe shutdowns. This count is incremented when a
     * shutdown notification (CC.SHN) is not received prior to loss of power.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } UnsafeShutdowns;

    /*
     * Contains the number of occurrences where the controller detected an
     * unrecovered data integrity error. Errors such as uncorrectable ECC, CRC
     * checksum failure, or LBA tag mismatch are included in this field.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } MediaErrors;

    /*
     * Contains the number of Error Information log entries over the life of the
     * controller.
     */
    struct
    {
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Lower;
        
        struct
        {
            ULONG DW0;
            ULONG DW1;
        }Upper;
    } NumberofErrorInformationLogEntries;

    UCHAR Reserved2[320];
} ADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY,
  *PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY;
#pragma pack(pop, smart_log)

typedef struct _FIRMWARE_SLOT_INFORMATION
{
    ULONG DW0;
    ULONG DW1;
}FRS;

/*
 * Get Log Page - Firmware Slot Information
 *
 * Section 5.10.1.3, Figure 560, (Log Identifier 0x03)
 */
typedef struct _ADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY
{
    /*
     * [Active Firmware Info] Specifies information about the active firmware
     * revision.
     */
    struct
    {
        /*
         * Bits 2:0 indicates the firmware slot that is contains the actively
         * running firmware revision.
         */
        UCHAR FirmwareSlot  :3;
        UCHAR Reserved1      :1;
        UCHAR NextTimetoActive :3;
        UCHAR Reserved2         :1;
    } AFI;

    UCHAR       Reserved1[7];

    /*
     * [Firmware Revision for Slot n+1] Contains the revision of the firmware
     * downloaded to firmware slot n+1. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    UCHAR Frs[7][8];
    
    UCHAR       Reserved2[448];
} ADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY,
  *PADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY;


/* Identify - Controller or NameSpace Structure Identifiers, Section 5.11, Figure 82 */
#define IDENTIFY_NAMESPACE_STRUCTURE           0x00
#define IDENTIFY_CONTROLLER_STRUCTURE          0x01
#define IDENTIFY_ACTIVE_NAMESPACE_LIST         0x02
#define IDENTIFY_NAMESPACE_ID_DESCRIPTOR       0x03

/* Identify - Power State Descriptor Data Structure, Section 5.11, Figure 66 */
typedef struct _ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR
{
    /*
     * [Maximum Power] This field indicates the maximum power consumed by the
     * NVM subsystem in this power state. The power in Watts is equal to the
     * value in this field multiplied by 0.01.
     */
    USHORT  MP;
    UCHAR   Reserved1;
    /*
    * [Max Power Scale] This field indicates the scale for the Maximum Power
    * field. If this field is cleared to '0', then the scale of the Maximum Power
    * field is in 0.01 Watts. If this field is set to '1', then the scale of the
    * Maximum Power field is in 0.0001 Watts.
    */
    UCHAR   MXPS :1;
    /*
    * [Non-Operational State] This field indicates whether the controller processes
    * I/O commands in this power state. If this field is cleared to '0', then the controller
    * processes I/O commands in this power state. If this field is set to '1', then
    * the controller does not process I/O commands in this power state. Refer to
    * section 8.4.1.
    */
    UCHAR   NOPS :1;
    UCHAR   Reserved2 :6;

    /*
     * [Entry Latency] This field indicates the maximum entry latency in
     * microseconds associated with entering this power state.
     */
    ULONG   ENLAT;

    /*
     * [Exit Latency] This field indicates the maximum exit latency in
     * microseconds associated with entering this power state.
     */
    ULONG   EXLAT;

    /*
     * [Relative Read Throughput] This field indicates the relative read
     * throughput associated with this power state. The value in this field
     * shall be less than the number of supported power states (e.g., if the
     * controller supports 16 power states, then valid values are 0 through 15).
     * A lower value means higher read throughput.
     */
    UCHAR   RRT       :5;
    UCHAR   Reserved3 :3;
    /*
     * [Relative Read Latency] This field indicates the relative read latency
     * associated with this power state. The value in this field shall be less
     * than the number of supported power states (e.g., if the controller
     * supports 16 power states, then valid values are 0 through 15). A lower
     * value means lower read latency.
     */
    UCHAR   RRL       :5;
    UCHAR   Reserved4 :3;
    /*
     * [Relative Write Throughput] This field indicates the relative write
     * throughput associated with this power state. The value in this field
     * shall be less than the number of supported power states (e.g., if the
     * controller supports 16 power states, then valid values are 0 through 15).
     * A lower value means higher write throughput.
     */
    UCHAR   RWT       :5;
    UCHAR   Reserved5 :3;
    /*
     * [Relative Write Latency] This field indicates the relative write latency
     * associated with this power state. The value in this field shall be less
     * than the number of supported power states (e.g., if the controller
     * supports 16 power states, then valid values are 0 through 15). A lower
     * value means lower write latency.
     */
    UCHAR   RWL       :5;
    UCHAR   Reserved6 :3;

    /*
    * [Idle Power] This field indicates the typical power consumed
    * by the NVM subsystem over  30  seconds  in  this  power  state  when
    * idle  (i.e.,  there  are  no  pending  commands,  register accesses,
    * background processes, nor device self-test operations). The
    * measurement starts after the NVM subsystem has been idle for
    * 10 seconds. The power in Watts is equal to the value in this field
    * multiplied by the scale indicated in the Idle Power Scale field. A value
    * of 0000h indicates Idle Power is not reported.
    */
    USHORT  IDLP;
    UCHAR   Reserved7 :6;
    /*
    * [Idle Power Scale] This field indicates the scale for the Idle Power field.
    */
    UCHAR   IPS :2;
    UCHAR   Reserved8;

    /*
    * [Active Power] This field indicates the largest average power
    * consumed by the NVM subsystem over a 10 second period in this power
    * state with the workload indicated in the Active Power Workload field.
    * The power in Watts is equal to the value in this field multiplied by the
    * scale indicated in the Active Power Scale field. A value of 0000h indicates
    * Active Power is not reported.
    */
    USHORT  ACTP;
    /*
    * [Active Power Workload] This field indicates the workload used to
    * calculate maximum power for this power state. Refer to section 8.4.3
    * for more details on each of the defined workloads. This field shall not
    * be "No Workload" unless ACTP is 0000h.
    */
    UCHAR   APW :3;
    UCHAR   Reserved9 :3;
    /*
    * [Active Power Scale] This field indicates the scale for the Active Power
    * field. If an Active Power Workload is reported for a power state, then the
    * Active Power Scale shall also be reported for that power state.
    */
    UCHAR   APS :2;
    UCHAR   Reserved10;

    ULONG   Reserved11[2];
} ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR,
  *PADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR;


/* Identify Controller Data Structure, Section 5.11, Figure 65 */
typedef struct _ADMIN_IDENTIFY_CONTROLLER
{
    /* Controller Capabiliites and Features */

    /*
     * [PCI Vendor ID] Contains the company vendor identifier that is assigned
     * by the PCI SIG. This is the same value as reported in the ID register in
     * section 2.1.1.
     */
    USHORT  VID;

    /*
     * [PCI Subsystem Vendor ID] Contains the company vendor identifier that is
     * assigned by the PCI SIG for the subsystem. This is the same value as
     * reported in the SS register in section 2.1.17.
     */
    USHORT  SSVID;

    /*
     * [Serial Number] Contains the serial number for the NVM subsystem that is
     * assigned by the vendor as an ASCII string. Refer to section 7.7 for
     * unique identifier requirements
     */
    UCHAR   SN[20];

    /*
     * [Model Number] Contains the model number for the NVM subsystem that is
     * assigned by the vendor as an ASCII string. Refer to section 7.7 for
     * unique identifier requirements.
     */
    UCHAR   MN[40];

    /*
     * [Firmware Revision] Contains the currently active firmware revision for
     * the NVM subsystem. This is the same revision information that may be
     * retrieved with the Get Log Page command, refer to section 5.10.1.3. See
     * section 1.8 for ASCII string requirements.
     */
    UCHAR   FR[8];

    /*
     * [Recommended Arbitration Burst] This is the recommended Arbitration Burst
     * size. Refer to section 4.7.
     */
    UCHAR   RAB;

    /*
     * IEEE OUI Identifier (IEEE): Contains the Organization Unique Identifier (OUI) for
     * the controller vendor. The OUI shall be a valid IEEE/RAC
     * ( assigned identifier that may be registered at
     * http://standards.ieee.org/develop/regauth/oui/public.html.
     * and Multi-Interface Capabilities
    */
    UCHAR IEEE[3];
    UCHAR MIC;
    /*
     *  Maximum Data Transfer Size(MDTS)
    */
    UCHAR MDTS;

    UCHAR Reserved11[6];

    U32 RTD3R;

    U32 RTD3E;
    
    UCHAR Reserved1[164];

    /* Admin Command Set Attributes */

    /*
     * [Optional Admin Command Support] This field indicates the optional Admin
     * commands supported by the controller. Refer to section 5.
     */
    struct
    {
        /*
         * Bit 0 if set to '1' then the controller supports the Security Send
         * and Security Receive commands. If cleared to '0' then the controller
         * does not support the Security Send and Security Receive commands.
         */
        USHORT  SupportsSecuritySendSecurityReceive     :1;

        /*
         * Bit 1 if set to '1' then the controller supports the Format NVM
         * command. If cleared to '0' then the controller does not support the
         * Format NVM command.
         */
        USHORT  SupportsFormatNVM                       :1;

        /*
         * Bit 2 if set to '1' then the controller supports the Firmware
         * Activate and Firmware Download commands. If cleared to '0' then the
         * controller does not support the Firmware Activate and Firmware
         * Download commands.
         */
        USHORT  SupportsFirmwareActivateFirmwareDownload:1;
        USHORT  Reserved                                :13;
    } OACS;

    /*
     * [Abort Command Limit] This field is used to convey the maximum number of
     * concurrently outstanding Abort commands supported by the controller (see
     * section 5.1). This is a 0’s based value. It is recommended that
     * implementations support a minimum of four Abort commands outstanding
     * simultaneously.
     */
    UCHAR ACL;

    /*
     * [Asynchronous Event Request Limit] This field is used to convey the
     * maximum number of concurrently outstanding Asynchronous Event Request
     * commands supported by the controller (see section 5.2). This is a 0’s
     * based value. It is recommended that implementations support a minimum of
     * four Asynchronous Event Request Limit commands oustanding simultaneously.
     */
    UCHAR   UAERL;

    /*
     * [Firmware Updates] This field indicates capabilities regarding firmware
     * updates. Refer to section 8.1 for more information on the firmware update
     * process.
     */
    struct
    {
        /*
         * Bit 0 if set to '1' indicates that the first firmware slot (slot 1)
         * is read only. If cleared to '0' then the first firmware slot (slot 1)
         * is read/write. Implementations may choose to have a baseline read
         * only firmware image.
         */
        UCHAR   FirstFirmwareSlotReadOnly               :1;

        /*
         * Bits 3:1 indicate the number of firmware slots that the device
         * supports. This field shall specify a value between one and seven,
         * indicating that at least one firmware slot is supported and up to
         * seven maximum. This corresponds to firmware slots 1 through 7.
         */
        UCHAR   SupportedNumberOfFirmwareSlots          :3;
        UCHAR   Reserved                                :4;
    } FRMW;

    /*
     * [Log Page Attributes] This field indicates optional attributes for log
     * pages that are accessed via the Get Log Page command.
     */
    struct
    {
        /*
         * Bit 0 if set to '1' then the controller supports the SMART / Health
         * information log page on a per namespace basis. If cleared to '0' then
         * the controller does not support the SMART / Health information log
         * page on a per namespace basis; the log page returned is global for
         * all namespaces.
         */
        UCHAR   SupportsSMARTLogPageOnNamespace  :1;
        UCHAR   SupportsCommandEffectsLogPage           :1;
        UCHAR   SupportsExtendedDataforGetLogPage       :1;
        UCHAR   Reserved                                :5;
    } LPA;

    /*
     * [Error Log Page Entries] This field indicates the number of Error
     * Information log entries that are stored by the controller. This field is
     * a 0's based value.
     */
    UCHAR   ELPE;

    /*
     * [Number of Power States Support] This field indicates the number of
     * NVMHCI power states supported by the controller. This is a 0's based
     * value. Refer to section 8.4. Power states are numbered sequentially
     * starting at power state 0. A controller shall support at least one power
     * state (i.e., power state 0) and may support up to 31 additional power
     * states (i.e., up to 32 total).
     */
    UCHAR NPSS;
    /*
     * Admin Vendor Specific Command Configuration (AVSCC): This field indicates
     * the configuration settings for admin vendor specific command handling.
     */
    UCHAR   AVSCC          :1;
    UCHAR   Reserved_AVSCC :7;

    /*
     * Autonomous Power State Transition Attributes (APSTA): 
     * This field indicates the attributes of the autonomous power state transition
     * feature. Bits 7:1 are reserved.
     */
    UCHAR   APSTA          :1;
    UCHAR   Reserved_APSTA :7;

    /*
      * Warning Composite Temperature Threshold (WCTEMP): 
      * This field indicates the minimum Composite Temperature field value
      * (reported in the SMART/Health Information log in Figure 79) that indicates
      * an overheating condition during which controller operation continues. Immediate
      * remediation is recommended (e.g., additional cooling or workload reduction).
      * The platfom should strive to maintain a composite temperature below this value. 
      * A value of 0h in this field indicates that no warning temperature threshold value
      * is reported by the controller. Implementations compliant to revision 1.2 or later
      * of this specification shall report a non-zero value in this field. It is recommended
      * that implementations report a value of 0157h in this field.
      */
    USHORT  WCTEMP;

    /*
      * Critical Composite Temperature Threshold (CCTEMP):
      * This field indicates the minimum Composite Temperature field value
      * (reported in the SMART/Health Information log in Figure 79) that indicates
      * a critical overheating condition (e.g., may prevent continued normal operation,
      * possibility of data loss, automatic device shutdown, extreme peformance
      * throttling, or permanent damage).
      * A value of 0h in this field indicates that no critical temperature threshold
      * value is reported by the controller. Implementations compliant to revision
      * 1.2 or later of this specification shall report a non-zero value in this field.
      */
    USHORT  CCTEMP;

    UCHAR   Reserved2[242];
    /* NVM Command Set Attributes */

    /*
     * [Submission Queue Entry Size] This field defines the required and maximum
     * Submission Queue entry size when using the NVM Command Set.
     */
    struct
    {
        /*
         * Bits 3:0 define the required Submission Queue Entry size when using
         * the NVM Command Set. This is the minimum entry size that may be used.
         * The value is in bytes and is reported as a power of two (2^n). The
         * required value shall be 6, corresponding to 64.
         */
        UCHAR   RequiredSubmissionQueueEntrySize        :4;

        /*
         * Bits 7:4 define the maximum Submission Queue entry size when using
         * the NVM Command Set. This value is larger than or equal to the
         * required SQ entry size. The value is in bytes and is reported as a
         * power of two (2^n).
         */
        UCHAR   MaximumSubmissionQueueEntrySize         :4;
    } SQES;

    /*
     * [Completion Queue Entry Size] This field defines the required and maximum
     * Completion Queue entry size when using the NVM Command Set.
     */
    struct
    {
        /*
         * Bits 3:0 define the required Completion Queue entry size when using
         * the NVM Command Set. This is the minimum entry size that may be used.
         * The value is in bytes and is reported as a power of two (2^n). The
         * required value shall be 4, corresponding to 16.
         */
        UCHAR   RequiredCompletionQueueEntrySize        :4;

        /*
         * Bits 7:4 define the maximum Completion Queue entry size when using
         * the NVM Command Set. This value is larger than or equal to the
         * required CQ entry size. The value is in bytes and is reported as a
         * power of two (2^n).
         */
        UCHAR   MaximumCompletionQueueEntrySize         :4;
    } CQES;

    UCHAR   Reserved3[2];

    /*
     * [Number of Namespaces] This field defines the number of valid namespaces
     * present for the controller.  Namespaces shall be allocated in order
     * (starting with 1) and packed sequentially.
     */
    ULONG   NN;

    /*
     * [Optional NVM Command Support] This field indicates the optional NVM
     * commands supported by the controller. Refer to section 6.
     */
    struct
    {
        /*
         * Bit 0 if set to '1' then the controller supports the Compare command.
         * If cleared to '0' then the controller does not support the Compare
         * command.
         */
        USHORT  SupportsCompare                         :1;

        /*
         * Bit 1 if set to '1' then the controller supports the Write
         * Uncorrectable command. If cleared to '0' then the controller does not
         * support the Write Uncorrectable command.
         */
        USHORT  SupportsWriteUncorrectable              :1;

        /*
         * Bit 2 if set to '1' then the controller supports the Dataset
         * Management command. If cleared to '0' then the controller does not
         * support the Dataset Management command.
         */
        USHORT  SupportsDataSetManagement               :1;

        /*
         * Bit 3 if set to ¡®1¡¯ then the controller supports the Write Zeroes 
         * command.  If cleared to ¡®0¡¯ then the controller does not support 
         * the Write Zeroes command.
         */
        USHORT  SupportsWriteZero                       :1;

        /*
         * Bit  4  if set to ¡®1¡¯ then the controller supports the Save field 
         * in the Set Features command and the Select field in the Get Features 
         * command.  If cleared to ¡®0¡¯ then the controller does not support 
         * the Save field in the Set Features command and the Select field in 
         * the Get Features command.
         */
        USHORT  SupportSetFeatureSave                   :1;

        /*
         * Bit 5 if set to ¡®1¡¯ then the controller supports reservations.  
         * If cleared to ¡®0¡¯then the controller does not support reservations.
         * If the controller supports reservations, then it shall support the 
         * following commands associated with reservations: Reservation Report, 
         * Reservation Register, Reservation Acquire, and Reservation Release. 
         * Refer to section 8.8 for additional requirements.
         */
        USHORT  SupportReservations                     :1;
        USHORT  Reserved                                :10;
    } ONCS;

    /*
     * [Fused Operation Support] This field indicates the fused operations that
     * the controller supports. Refer to section 6.1.
     */
    struct
    {
        /*
         * Bit 0 if set to '1' then the controller supports the Compare and
         * Write fused operation. If cleared to '0' then the controller does not
         * support the Compare and Write fused operation. Compare shall be the
         * first command in the sequence.
         */
        USHORT  SupportsCompare_Write                   :1;
        USHORT  Reserved                                :15;
    } FUSES;

    /*
     * [Format NVM Attributes] This field indicates attributes for the Format
     * NVM command.
     */
    struct
    {
        /*
         * Bit 0 indicates whether the format operation applies to all
         * namespaces or is specific to a particular namespace. If set to '1'
         * then all namespaces shall be configured with the same attributes and
         * a format of any namespace results in a format of all namespaces. If
         * cleared to '0' then the controller supports format on a per
         * namespace basis.
         */
        UCHAR   FormatAppliesToAllNamespaces            :1;

        /*
         * Bit 1 indicates whether secure erase functionality applies to all
         * namespaces or is specific to a particular namespace. If set to '1'
         * then a secure erase of a particular namespace as part of a format
         * results in a secure erase of all namespaces. If cleared to '0' then
         * a secure erase as part of a format is performed on a per namespace
         * basis.
         */
        UCHAR   SecureEraseAppliesToAllNamespaces       :1;

        /*
         * Bit 2 indicates whether cryptographic erase is supported as part of
         * the secure erase functionality. If set to '1' then cryptographic
         * erase is supported. If cleared to '0' then cryptographic erase is
         * not supported.
         */
        UCHAR   SupportsCryptographicErase              :1;
        UCHAR   Reserved                                :5;
    } FNA;

    /*
     * [Volatile Write Cache] This field indicates attributes related to the
     * presence of a volatile write cache in the implementation.
     */
    struct
    {
        /*
         * Bit 0 if set to '1' indicates that a volatile write cache is present.
         * If cleared to '0' a volatile write cache is not present. If a
         * volatile write cache is present, then the host may issue Flush
         * commands and control whether it is enabled with Set Features
         * specifying the Volatile Write Cache feature identifier. If a volatile
         * write cache is not present, the host shall not issue Flush commands
         * nor Set Features or Get Features with the Volatile Write Cache
         * identifier.
         */
        UCHAR   Present               :1;
        UCHAR   Reserved                                :7;
    } VWC;

    /*
     * [Atomic Write Unit Normal] This field indicates the atomic write size for
     * the controller during normal operation. This field is specified in
     * logical blocks and is a 0's based value. If a write is issued of this
     * size or less, the host is guaranteed that the write is atomic to the
     * NVM with respect to other read or write operations. A value of FFh
     * indicates all commands are atomic as this is the largest command size. It
     * is recommended that implementations support a minimum of 128KB
     * (appropriately scaled based on LBA size).
     */
    USHORT  AWUN;

    /*
     * [Atomic Write Unit Power Fail] This field indicates the atomic write size
     * for the controller during a power fail condition. This field is specified
     * in logical blocks and is a 0's based value. If a write is issued of this
     * size or less, the host is guaranteed that the write is atomic to the NVM
     * with respect to other read or write operations.
     */
    USHORT  AWUPF;
    /*
     * NVM Vendor Specific Command Configuration (NVSCC): This field indicates
     * the configuration settings for NVM vendor specific command handling.
     */
    UCHAR   NVSCC          :1;
    UCHAR   Reserved_NVSCC :7;
    UCHAR   Reserved4[173];
    /* I/O Command Set Attributes */
    UCHAR   Reserved5[1344];

    /* Power State Descriptors */

    /*
     * [Power State x Descriptor] This field indicates the characteristics of
     * power state x. The format of this field is defined in Figure 66.
     */
    ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR PSD[32];

    /* Vendor Specific */

    /*
     * [Vendor Specific] This range of bytes is allocated for vendor specific
     * usage.
     */
    UCHAR   VS[1024];
} ADMIN_IDENTIFY_CONTROLLER, *PADMIN_IDENTIFY_CONTROLLER;


/*
 * Identify - LBA Format Data Structure, NVM Command Set Specific
 *
 * Section 5.11, Figure 68
 */
typedef struct _ADMIN_IDENTIFY_FORMAT_DATA
{
    /*
     * [Metadata Size] This field indicates the number of metadata bytes
     * provided per LBA based on the LBA Size indicated. The namespace may
     * support the metadata being transferred as part of an extended data LBA or
     * as part of a separate contiguous buffer. If end-to-end data protection is
     * enabled, then the first eight bytes or last eight bytes of the metadata
     * is the protection information.
     */
    USHORT  MS;

    /*
     * [LBA Data Size] This field indicates the LBA data size supported. The
     * value is reported in terms of a power of two (2^n). A value smaller than
     * 9 (i.e. 512 bytes) is not supported. If the value reported is 0h then the
     * LBA format is not supported / used.
     */
    UCHAR   LBADS;

    /*
     * [Relative Performance] This field indicates the relative performance of
     * the LBA format indicated relative to other LBA formats supported by the
     * controller. Depending on the size of the LBA and associated metadata,
     * there may be performance implications. The performance analysis is based
     * on better performance on a queue depth 32 with 4KB read workload. The
     * meanings of the values indicated are included in the following table.
     * Value 00b == Best performance. Value 01b == Better performance. Value 10b
     * == Good performance. Value 11b == Degraded performance.
     */
    UCHAR   RP       :2;
    UCHAR   Reserved :6;
} ADMIN_IDENTIFY_FORMAT_DATA, *PADMIN_IDENTIFY_FORMAT_DATA;


/* Identify Namespace Data Structure, Section 5.11, Figure 67 */
typedef struct _ADMIN_IDENTIFY_NAMESPACE
{
    /*
     * [Namespace Size] This field indicates the total size of the namespace in
     * logical blocks. A namespace of size n consists of LBA 0 through (n - 1).
     * The number of logical blocks is based on the formatted LBA size. This
     * field is undefined prior to the namespace being formatted. Note: The
     * creation of the namespace(s) and initial format operation are outside the
     * scope of this specification.
     */
    ULONGLONG                   NSZE;

    /*
     * [Namespace Capacity] This field indicates the maximum number of logical
     * blocks that may be allocated in the namespace at any point in time. The
     * number of logical blocks is based on the formatted LBA size. This field
     * is undefined prior to the namespace being formatted. This field is used
     * in the case of thin provisioning and reports a value that is smaller than
     * or equal to the Namespace Size. Spare LBAs are not reported as part of
     * this field. A value of 0h for the Namespace Capacity indicates that the
     * namespace is not available for use. A logical block is allocated when it
     * is written with a Write or Write Uncorrectable command. A logical block
     * may be deallocated using the Dataset Management command.
     */
    ULONGLONG                   NCAP;

    /*
     * [Namespace Utilization] This field indicates the current number of
     * logical blocks allocated in the namespace. This field is smaller than or
     * equal to the Namespace Capacity. The number of logical blocks is based on
     * the formatted LBA size. When using the NVM command set: A logical block
     * is allocated when it is written with a Write or Write Uncorrectable
     * command. A logical block may be deallocated using the Dataset Management
     * command.
     */
    ULONGLONG                   NUSE;

    /* [Namespace Features] This field defines features of the namespace. */
    struct
    {
        /*
         * Bit 0 if set to '1' indicates that the namespace supports thin
         * provisioning. Specifically, the Namespace Capacity reported may be
         * less than the Namespace Size. When this feature is supported and the
         * Dataset Management command is supported then deallocating LBAs shall
         * be reflected in the Namespace Utilization field. Bit 0 if cleared to
         * '0' indicates that thin provisioning is not supported and the
         * Namespace Size and Namespace Capacity fields report the same value.
         */
        UCHAR   SupportsThinProvisioning    :1;
        UCHAR   Reserved                    :7;
    } NSFEAT;

    /*
     * [Number of LBA Formats] This field defines the number of supported LBA
     * size and metadata size combinations supported by the namespace. LBA
     * formats shall be allocated in order (starting with 0) and packed
     * sequentially. This is a 0's based value. The maximum number of LBA
     * formats that may be indicated as supported is 16. The supported LBA
     * formats are indicated in bytes 128 ?191 in this data structure. The
     * metadata may be either transferred as part of the LBA (creating an
     * extended LBA which is a larger LBA size that is exposed to the
     * application) or it may be transferred as a separate contiguous buffer of
     * data. The metadata shall not be split between the LBA and a separate
     * metadata buffer. It is recommended that software and controllers
     * transition to an LBA size that is 4KB or larger for ECC efficiency at the
     * controller. If providing metadata, it is recommended that at least 8
     * bytes are provided per logical block to enable use with end-to-end data
     * protection, refer to section 8.2.
     */
    UCHAR NLBAF;

    /*
     * [Formatted LBA Size] This field indicates the LBA size & metadata size
     * combination that the namespace has been formatted with.
     */
    struct
    {
        /*
         * Bits 3:0 indicates one of the 16 supported combinations indicated in
         * this data structure. This is a 0’s based value.
         */
        UCHAR   SupportedCombination        :4;

        /*
         * Bit 4 if set to '1' indicates that the metadata is transferred at the
         * end of the data LBA, creating an extended data LBA. Bit 4 if cleared
         * to '0' indicates that all of the metadata for a command is transferred
         * as a separate contiguous buffer of data.
         */
        UCHAR   SupportsMetadataAtEndOfLBA  :1;
        UCHAR   Reserved                    :3;
    } FLBAS;

    /*
     * [Metadata Capabilities] This field indicates the capabilities for
     * metadata.
     */
    struct
    {
        /*
         * Bit 0 if set to '1' indicates that the namespace supports the
         * metadata being transferred as part of an extended data LBA.
         * Specifically, the metadata is transferred as part of the data PRP
         * Lists. Bit 0 if cleared to '0' indicates that the namespace does not
         * support the metadata being transferred as part of an extended data
         * LBA.
         */
        UCHAR   SupportsMetadataAsPartOfLBA :1;

        /*
         * Bit 1 if set to '1' indicates the namespace supports the metadata
         * being transferred as part of a separate buffer that is specified in
         * the Metadata Pointer. Bit 1 if cleared to '0' indicates that the
         * controller does not support the metadata being transferred as part of
         * a separate buffer.
         */
        UCHAR   SupportsMetadataAsSeperate  :1;
        UCHAR   Reserved                    :6;
    } MC;

    /*
     * [End-to-end Data Protection Capabilities] This field indicates the
     * capabilities for the end-to-end data protection feature. Multiple bits
     * may be set in this field. Refer to section 8.3.
     */
    struct {
        /*
         * Bit 0 if set to '1' indicates that the namespace supports Protection
         * Information Type 1. Bit 0 if cleared to '0' indicates that the
         * namespace does not support Protection Information Type 1.
         */
        UCHAR   SupportsProtectionType1     :1;

        /*
         * Bit 1 if set to '1' indicates that the namespace supports Protection
         * Information Type 2. Bit 1 if cleared to '0' indicates that the
         * namespace does not support Protection Information Type 2.
         */
        UCHAR   SupportsProtectionType2     :1;

        /*
         * Bit 2 if set to ??indicates that the namespace supports Protection
         * Information Type 3. Bit 2 if cleared to ??indicates that the
         * namespace does not support Protection Information Type 3.
         */
        UCHAR   SupportsProtectionType3     :1;

        /*
         * Bit 3 if set to ??indicates that the namespace supports protection
         * information transferred as the first eight bytes of metadata. Bit 3
         * if cleared to ??indicates that the namespace does not support
         * protection information transferred as the first eight bytes of
         * metadata.
         */
        UCHAR   SupportsProtectionFirst8    :1;

        /*
         * Bit 4 if set to ??indicates that the namespace supports protection
         * information transferred as the last eight bytes of metadata. Bit 4 if
         * cleared to ??indicates that the namespace does not support
         * protection information transferred as the last eight bytes of
         * metadata.
         */
        UCHAR   SupportsProtectionLast8     :1;
        UCHAR   Reserved                    :3;
    } DPC;

    /*
     * [End-to-end Data Protection Type Settings] This field indicates the Type
     * settings for the end-to-end data protection feature. Refer to section
     * 8.3.
     */
    struct
    {
        /*
         * Bits 2:0 indicate whether Protection Information is enabled and the
         * type of Protection Information enabled. The values for this field
         * have the following meanings: Value 000b == Protection information is
         * not enabled. Value 001b == Protection information is enabled, Type 1.
         * Value 010b == Protection information is enabled, Type 2. Value 011b
         * == Protection information is enabled, Type 3. Value 100b-111b ==
         * Reserved.
         */
        UCHAR   ProtectionEnabled           :3;

        /*
         * Bit 3 if set to ??indicates that the protection information, if
         * enabled, is transferred as the first eight bytes of metadata. Bit 3
         * if cleared to 0?indicates that the protection information, if
         * enabled, is transferred as the last eight bytes of metadata.
         */
        UCHAR   ProtectionInFirst8          :1;
        UCHAR   Reserved                    :4;
    } DPS;

    UCHAR                       Reserved1[98];

    /*
     * [LBA Format x Support] This field indicates the LBA format x that is
     * supported by the controller. The LBA format field is defined in Figure
     * 68.
     */
    ADMIN_IDENTIFY_FORMAT_DATA  LBAFx[NUM_LBAF];
    UCHAR                       Reserved2[192];

    /*
     * [Vendor Specific] This range of bytes is allocated for vendor specific
     * usage.
     */
    UCHAR                       VS[3712];
} ADMIN_IDENTIFY_NAMESPACE, *PADMIN_IDENTIFY_NAMESPACE;


/*
 * Get Features - Features Identifiers
 *
 * Section 5.9, Figure 53, Figure 54; Section 5.12.1, Figure 72, Figure 73
 */
#define FEATURE_LIST                                \
    LL(ARBITRATION                         ,0x01)   \
    LL(POWER_MANAGEMENT                    ,0x02)   \
    LL(LBA_RANGE_TYPE                      ,0x03)   \
    LL(TEMPERATURE_THRESHOLD               ,0x04)   \
    LL(ERROR_RECOVERY                      ,0x05)   \
    LL(VOLATILE_WRITE_CACHE                ,0x06)   \
    LL(NUMBER_OF_QUEUES                    ,0x07)   \
    LL(INTERRUPT_COALESCING                ,0x08)   \
    LL(INTERRUPT_VECTOR_CONFIGURATION      ,0x09)   \
    LL(WRITE_ATOMICITY                     ,0x0A)   \
    LL(ASYNCHRONOUS_EVENT_CONFIGURATION    ,0x0B)   \
    LL(SOFTWARE_PROGRESS_MARKER            ,0x80)

#define LL(a,b) a = b,
enum
{
    FEATURE_LIST
};
#undef LL

#define LL(a,b) a##_NUM,
enum
{
    FEATURE_LIST
    FEATURE_NUM
};
#undef LL


/*
 * LBA Range Type - Entry
 *
 * Section 5.12.1.3, Figure 77, Feature Identifier 03h
 */
typedef struct _ADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY
{
    /*
     * [Type] Identifies the Type of the LBA range. The Types are listed below.
     * Value 00h == Reserved.  Value 01h == Filesystem. Value 02h == RAID. Value
     * 03h == Cache. Value 04h == Page/swap file.  Value 05h-7Fh == Reserved.
     * Value 80h-FFh == Vendor Specific.
     */
    UCHAR       Type;

    /* Identifies attributes of the LBA range. Each bit defines an attribute. */
    struct
    {
        /*
         * If set to ?? the LBA range may be overwritten. If cleared to ??
         * the area should not be overwritten.
         */
        UCHAR   Overwriteable   :1;

        /*
         * If set to ?? the LBA range should be hidden from the OS / EFI /
         * BIOS. If cleared to ?? the area should be visible to the OS / EFI
         * / BIOS.
         */
        UCHAR   Hidden          :1;
        UCHAR   Reserved        :6;
    } Attributes;

    UCHAR       Reserved1[14];

    /*
     * [Starting LBA] This field indicates the 64-bit address of the first LBA
     * that is part of this LBA range.
     */
    ULONGLONG   SLBA;

    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks that are part of this LBA range. This is a 0’s based value.
     */
    ULONGLONG   NLB;

    /*
     * [Unique Identifier] This field is a global unique identifier that
     * uniquely identifies the type of this LBA range. Well known Types may be
     * defined and are published on the NVMHCI website.
     */
    UCHAR       GUID[16];
    UCHAR       Reserved2[16];
} ADMIN_SET_FEATURES_LBA_COMMAND_RANGE_TYPE_ENTRY,
  *PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY;

typedef struct _CCC_CONTROL
{
    U32 MX0CCCEnable:1;
    U32 MX1CCCEnable:1;
    U32 MX2CCCEnable:1;
    U32 MX3CCCEnable:1;
    U32 MX4CCCEnable:1;
    U32 MX5CCCEnable:1;
    U32 MX6CCCEnable:1;
    U32 MX7CCCEnable:1;
    U32 MX8CCCEnable:1;
    U32 MXINTDelay:1;
    U32 TMSCL:2;
    U32 Reserved:4;
    U32 THR:8;
    U32 TIME:8;
}CCC_CONTROL;

typedef union _ADMIN_CMD_FORMAT_NVM
{
    U32 DW;
    struct {
        U32 LBAF:4;
        U32 MS:1;
        U32 PI:3;
        U32 PIL:1;
        U32 SES:3;
        U32 Rsv:20;
    };
}ADMIN_CMD_FORMAT_NVM, *PADMIN_CMD_FORMAT_NVM;

#endif /* __NVME_H__ */
