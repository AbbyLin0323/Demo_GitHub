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
Filename    :L0_ATAGenericLib.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __L0_ATA_GENERIC_CMD_LIB_H__
#define __L0_ATA_GENERIC_CMD_LIB_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

/* VIA-SSD ATA Lib definition */
typedef enum _HCMD_TYPE
{
    HCMD_TYPE_SOFT_RESET = 0,
    HCMD_TYPE_DIRECT_MEDIA_ACCESS,
    HCMD_TYPE_NEED_SPECIAL_STATUS,
    HCMD_TYPE_OTHERS
} HCMD_TYPE;

typedef enum _SATA_PROTOCOL
{
    SATA_PROT_NONDATA = 0,
    SATA_PROT_DIAG,
    SATA_PROT_PIO,
    SATA_PROT_DMA,
    SATA_PROT_FPDMA
} SATA_PROTOCOL;

typedef enum _ATACMD_STATE
{
    ATACMD_NEW = 0,
    ATACMD_ABORTED,
    ATACMD_DATAUNC,
    ATACMD_SPLITTING,
    ATACMD_IDFY_WAITINGDATA,
    ATACMD_FLUSH_WAITING,
    ATACMD_SHUTDOWN_WAITING,
    ATACMD_SECURITY_WAITINGDATA,
    ATACMD_DSM_WAITINGDATA,
    ATACMD_DSM_PROCDATA,
    ATACMD_DSM_DISPATCH,
    ATACMD_DSM_WAITING,
    ATACMD_COMPLETED
} ATACMD_STATE;

/* ATA command code definition */
//Control Command
#define ATA_CMD_EXECUTE_DEVICE_DIAGNOSTIC    0x90

#define ATA_CMD_FLUSH_CACHE                  0xE7
#define ATA_CMD_FLUSH_CACHE_EXT              0xEA

#define ATA_CMD_IDENTIFY_DEVICE              0xEC
#define ATA_CMD_SET_FEATURES                 0xEF
#define ATA_CMD_SET_MULTIPLE_MODE            0xC6

#define ATA_CMD_READ_LOG_EXT                 0x2F
#define ATA_CMD_WRITE_LOG_EXT                0x3F
#define LOG_PAGE_DIRECTORY                   0x00
#define LOG_PAGE_NCQ_ERROR                   0x10

#define ATA_CMD_NOP                          0x00
#define ATA_CMD_SEEK                         0x70

//PowerManagment 
#define ATA_CMD_CHECK_POWER_MODE             0xE5
#define ATA_CMD_IDLE                         0xE3
#define ATA_CMD_IDLE_IMMEDIATE               0xE1
#define ATA_CMD_SLEEP                        0xE6
#define ATA_CMD_STANDBY                      0xE2
#define ATA_CMD_STANDBY_IMMEDIATE            0xE0

//Data Command
//PIO Data command
#define ATA_CMD_READ_SECTOR                  0x20
#define ATA_CMD_READ_SECTOR_EXT              0x24

#define ATA_CMD_WRITE_SECTOR                 0x30
#define ATA_CMD_WRITE_SECTOR_EXT             0x34

#define ATA_CMD_READ_MULTIPLE                0xC4
#define ATA_CMD_READ_MULTIPLE_EXT            0x29

#define ATA_CMD_WRITE_MULTIPLE               0xC5
#define ATA_CMD_WRITE_MULTIPLE_EXT           0x39

//DMA Data command
#define ATA_CMD_READ_DMA                     0xC8
#define ATA_CMD_READ_DMA_EXT                 0x25

#define ATA_CMD_WRITE_DMA                    0xCA
#define ATA_CMD_WRITE_DMA_EXT                0x35
#define ATA_CMD_DATA_SET_MANAGEMENT          0x06

//NCQ Data command
#define ATA_CMD_READ_FPDMA_QUEUED            0x60
#define ATA_CMD_WRITE_FPDMA_QUEUED           0x61

//NCQ NON-DATA command
#define ATA_CMD_NCQ_NON_DATA                 0x63
#define ATA_CMD_SEND_FPDMA_QUEUED            0x64
#define ATA_CMD_RECEIVE_FPDMA_QUEUED         0x65

//Other
#define ATA_CMD_READ_VERIFY_SECTOR           0x40
#define ATA_CMD_READ_VERIFY_SECTOR_EXT       0x42
#define ATA_CMD_IDENTIFY_PACKET_DEVICE       0xA1
#define ATA_CMD_READ_BUFFER                  0xE4
#define ATA_CMD_WRITE_BUFFER                 0xE8

//Option Command
//HPA Command
#define ATA_CMD_READ_NATIVE_MAX_ADDRESS      0xF8
#define ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT  0x27
#define ATA_CMD_SET_MAX_ADDRESS              0xF9
#define ATA_CMD_SET_MAX_ADDRESS_EXT          0x37

//Secure Command
#define ATA_CMD_SECURITY_DISABLE_PASSWORD    0xF6
#define ATA_CMD_SECURITY_ERASE_PREPARE       0xF3
#define ATA_CMD_SECURITY_ERASE_UNIT          0xF4
#define ATA_CMD_SECURITY_FREEZE_LOCK         0xF5
#define ATA_CMD_SECURITY_SET_PASSWORD        0xF1
#define ATA_CMD_SECURITY_UNLOCK              0xF2

//other
#define ATA_CMD_DOWNLOAD_MICROCODE           0x92
#define ATA_CMD_DOWNLOAD_MICROCODE_DMA           0x93
#define ATA_CMD_RECALIBRATE                  0x10

//old command
#define ATA_CMD_INITIALIZE_DEVICE_PARAMETERS 0x91

//SMART command
#define ATA_CMD_SMART                        0xB0
#define SMART_FEATURE_READ_DATA 0xD0 /*SMART read data*/
#define SMART_READ_ATTRIBUTE_THRESHOLDS 0xD1 /*SMART read thersholds*/
#define SMART_FEATURE_AUTOSAVE_DISEN 0xD2 /*SMART AutoSave Disable/Enable*/
#define SMART_EXEC_OFFLINE_IMMEDIATE 0xD4
#define SMART_READ_LOG   0xD5
#define SMART_CMD_WRITE_LOG 0xD6
#define SMART_ENABLE_OPERATIONS 0xD8
#define SMART_DISABLE_OPERATIONS 0xD9
#define SMART_RETURN_STATUS 0xDA

//vender define commands: for factory untility interface
#define ATA_CMD_VENDER_DEFINE                0xFF

//SetFeature Macro
#define ENABLE_WRITE_CACHE   0x02
#define DISABLE_WRITE_CACHE  0x82

//SetFeature Macro
#define ENABLE_READ_LOOK_AHEAD    0xAA
#define DISABLE_READ_LOOK_AHEAD   0x55

#define ENABLE_POWER_ON_DEFAULTS  0xCC
#define DISABLE_POWER_ON_DEFAULTS 0x66

#define SET_TRANSFER_MODE     0x03

#define ENABLE_APM            0x05
#define DISABLE_APM           0x85

#define ENABLE_SATA_FEATURE   0x10
#define DISABLE_SATA_FEATURE  0x90

#define SATA_FEATURE_NONZERO_OFFSET            0x1
#define SATA_FEATURE_AUTO_ACTIVATE             0x2
#define SATA_FEATURE_DIPM                      0x3
#define SATA_FEATURE_GUARANTEED_INORDER        0x4  
#define SATA_FEATURE_SETTING_PRESERVATION      0x6
#define SATA_FEATURE_DIAPTS                    0x7
#define SATA_FEATURE_DEVSLP                    0x9

#define SATA_POWER_ACTIVE         0x40
#define SATA_POWER_IDLE           0x80 
#define SATA_POWER_STANDBY        0x00
#define SATA_POWER_ACTIVEORIDLE   0xff
#define SATA_POWER_SLEEP          0x20     //can not find in PATA Spec

#define SATA_STANDBY_TIMER_PERIOD 10 // We use 10s as a time fragment.

//Subcommand code and macros defined for vendor defined command
#define VIA_TRACE_GETLOGINFO      0xC0
#define VIA_TRACE_GETLOGDATA      0xC1

#define DEVHEAD_LBA_BIT (1 << 6)

/* GPL definition */
#define ATA_GPL_BUFFSIZE                BUF_SIZE

//define sector offset in L0 internal DRAM buff
#define GPL_LOGDIR_SEC_OFF 0
#define GPL_NCQERR_SEC_OFF 1
#define GPL_IDFYDATA_SEC_OFF 2
#define GPL_SMART_EXTCOMP_OFF 11
#define GPL_EXT_SELF_TEST_OFF 13
#define GPL_SMART_HOST_SPEC_80H_OFF 15

#define GPL_LOGADDR_LOGDIR              0x00
#define GPL_LOGADDR_SMART_EXTCOMP       0x03
#define GPL_LOGADDR_EXTENDED_SELF_TEST  0x07
#define GPL_LOGADDR_NCQERR              0x10
#define GPL_LOGADDR_IDFYDATA            0x30

#define GPL_IDFYDATA_PAGE_LIST          0x0
#define GPL_IDFYDATA_PAGE_IDCOPY        0x1
#define GPL_IDFYDATA_PAGE_CAPACITY      0x2
#define GPL_IDFYDATA_PAGE_CAPABILITIES  0x3
#define GPL_IDFYDATA_PAGE_CURRSETTINGS  0x4
#define GPL_IDFYDATA_PAGE_ATASTRINGS    0x5
#define GPL_IDFYDATA_PAGE_SECURITY      0x6
#define GPL_IDFYDATA_PAGE_PATA          0x7
#define GPL_IDFYDATA_PAGE_SATA          0x8

/* The data structure describing which logs are currently supported. */
typedef struct _GPLD
{
    U32 ulLogAddr;
    U32 ulLogPageNum;
} GPLD, *PGPLD;

/* The data structure describing current CHS translation parameters. */
typedef struct _CHSPARAM
{
    U16 usNumOfHead;
    U16 usSecPerTrk;
    U32 ulCurrDRQSize;
} CHSPARAM, *PCHSPARAM;

/* The data structure describing the entire IDENTIFY DEVICE data area. */
typedef struct _IDENTIFY_DEVICE_DATA {
    struct {
        U16 Reserved1  :1;
        U16 Retired3  :1;
        U16 ResponseIncomplete  :1;
        U16 Retired2  :3;
        U16 FixedDevice  :1;
        U16 RemovableMedia  :1;
        U16 Retired1  :7;
        U16 DeviceType  :1;
    } GeneralConfiguration;//word 0
    U16 NumCylinders;//word 1
    U16 ReservedWord2;//word 2
    U16 NumHeads;//word 3
    U16 Retired1[2];//word 4~5
    U16 NumSectorsPerTrack;//word 6
    U16 VendorUnique1[3];//word 7~9
    U8  SerialNumber[20];//word 10~19
    U16 Retired2[2];//word 20~21
    U16 Obsolete1;//word 22
    U8  FirmwareRevision[8];//word 23~26
    U8  ModelNumber[40];//word 27~46
    U8  MaximumBlockTransfer; 
    U8  VendorUnique2; //word 47
    U16 ReservedWord48;//word 48--defined in ATA8-ACS-3
    struct {
        U8  ReservedByte49;
        U8  DmaSupported  :1;
        U8  LbaSupported  :1;
        U8  IordyDisable  :1;
        U8  IordySupported  :1;
        U8  Reserved1  :1;
        U8  StandybyTimerSupport  :1;
        U8  Reserved2  :2;
        U16 ReservedWord50;
    } Capabilities;//word 49~50
    U16 ObsoleteWords51[2];//word 51~52
    U16 TranslationFieldsValid  :3; 
    U16 Reserved3  :13;//word 53
    U16 NumberOfCurrentCylinders;//word 54
    U16 NumberOfCurrentHeads;//word 55
    U16 CurrentSectorsPerTrack;//word 56
    U16 CurrentSectorCapacityLow;//word 57
    U16 CurrentSectorCapacityHigh;//word 58
    U8  CurrentMultiSectorSetting;
    U8  MultiSectorSettingValid  :1;
    U8  ReservedByte59  :7; //word 59
    U32  UserAddressableSectors;//word 60~61
    U16 ObsoleteWord62;//word 62
    U16 MultiWordDMASupport  :8;
    U16 MultiWordDMAActive  :8;//word 63
    U16 AdvancedPIOModes  :2;
    U16 ReservedByte64  :14;//word 64
    U16 MinimumMWXferCycleTime;//word 65
    U16 RecommendedMWXferCycleTime;//word 66
    U16 MinimumPIOCycleTime;//word 67
    U16 MinimumPIOCycleTimeIORDY;//word 68
    struct{
        U16 Reserved :3;
        U16 SupportExtUserAddrSpace :1;
        U16 EncryptUserData :1;
        U16 SupportTrimLBARetZeroValue :1;
        U16 SupportOptionalATA28Cmd :1;
        U16 ReservedForIEEE1667 :1;
        U16 SupportDownloadMicroCodeDMA :1;
        U16 SupportSetMaxDMA :1;
        U16 SupportWriteBufferDMA :1;
        U16 SupportReadBufferDMA :1;
        U16 SupportDCODataDMA :1;
        U16 SupportLongPhySecAlgnErrRprtCtrl :1;
        U16 SupportTrimLBARetDtmValue :1;
        U16 SupportCFastSpec :1;
    } AdditionalSupported;//word 69
    U16 ReservedWords70[5];//word 70~74
    U16 QueueDepth  :5;
    U16 ReservedWord75  :11;//word 75
    struct{
        U16 ShallBeZero :1;
        U16 SataGen1Speed :1;
        U16 SataGen2Speed :1;
        U16 SataGen3Speed :1;
        U16 Reserved :4;//byte 0
        U16 NcqFeatureSet :1;
        U16 ReceiptHostInitPmReq: 1;
        U16 SataPhyEventCounterLog :1;
        U16 NcqOutstandingUnload :1;
        U16 NcqPriority :1;
        U16 HostAutoPartSlumber :1;
        U16 DevAutoPartSlumber :1;
        U16 ReadLogDmaExt :1; //Supports READ LOG DMA EXT as equivalent to READ LOG EXT
    }SataCapabilities;//word 76
    struct{
        U16 ShallBeZero :1;
        U16 CurNegotiatedSpeed :3;
        U16 NcqStreaming :1;
        U16 NcqNonData :1;
        U16 RcvSendFpdmaQueued :1;
        U16 DevSleepToReducedPwrSt :1;//byte 0
        U16 Reserved :8;
    }SataAdditionalCapbilities;//word 77
    struct{
        U16 ShallBeZero :1;
        U16 NonZeroOffset :1;
        U16 AutoActivate :1;
        U16 InitInterfacePm :1;
        U16 InOrderDelivery :1;
        U16 HwFeatureControl :1;
        U16 SwSettingPreserve :1;
        U16 NcqAotosense :1;//byte 0
        U16 DevSleep :1;
        U16 HybridInfo :1;
        U16 DipmSsp :1;
        U16 RebuildAssist :1;
        U16 Reserved1 :4;
    }SataFeatureSupport;//word 78
    struct{
        U16 ShallBeZero :1;
        U16 NonZeroOffsetEn :1;
        U16 AutoActivateEn :1;
        U16 DipmEn :1;//Device initiated power management enable
        U16 InOrderDeliveryEn :1;
        U16 HwFeatureControlEn :1;
        U16 SwSettingPreserveEn :1;//Software Settings Preservation enable
        U16 DevAutoPartSlumberEn :1;//byte 0
        U16 DevSleepEn :1;
        U16 HybridInfoEn :1;
        U16 Reserved :1;
        U16 RebuildAssistEn :1;
        U16 Reserved1 :4;
    }SataFeatureEnable;//word 79
    U16 MajorRevision;//word 80
    U16 MinorRevision;//word 81
    struct {
        U8 SmartCommands  :1;
        U8 SecurityMode  :1;
        U8 RemovableMediaFeature  :1;
        U8 PowerManagement  :1;
        U8 Reserved1  :1;
        U8 WriteCache  :1;
        U8 LookAhead  :1;
        U8 ReleaseInterrupt  :1;
        U8 ServiceInterrupt  :1;
        U8 DeviceReset  :1;
        U8 HostProtectedArea  :1;
        U8 Obsolete1  :1;
        U8 WriteBuffer  :1;
        U8 ReadBuffer  :1;
        U8 Nop  :1;
        U8 Obsolete2  :1;//word 82
        U8 DownloadMicrocode  :1;
        U8 DmaQueued  :1;
        U8 Cfa  :1;
        U8 AdvancedPm  :1;
        U8 Msn  :1;
        U8 PowerUpInStandby  :1;
        U8 ManualPowerUp  :1;
        U8 Reserved2  :1;
        U8 SetMax  :1;
        U8 Acoustics  :1;
        U8 BigLba  :1;
        U8 DeviceConfigOverlay  :1;
        U8 FlushCache  :1;
        U8 FlushCacheExt  :1;
        U8 Resrved3  :2;//word 83
        U8 SmartErrorLog  :1;
        U8 SmartSelfTest  :1;
        U8 MediaSerialNumber  :1;
        U8 MediaCardPassThrough  :1;
        U8 StreamingFeature  :1;
        U8 GpLogging  :1;
        U8 WriteFua  :1;
        U8 WriteQueuedFua  :1;
        U8 WWN64Bit  :1;
        U8 URGReadStream  :1;
        U8 URGWriteStream  :1;
        U8 ReservedForTechReport  :2;
        U8 IdleWithUnloadFeature  :1;
        U8 Reserved4  :2;//word 84
    } CommandSetSupport;
    struct {
        U8 SmartCommands  :1;
        U8 SecurityMode  :1;
        U8 RemovableMediaFeature  :1;
        U8 PowerManagement  :1;
        U8 Reserved1  :1;
        U8 WriteCache  :1;
        U8 LookAhead  :1;
        U8 ReleaseInterrupt  :1;
        U8 ServiceInterrupt  :1;
        U8 DeviceReset  :1;
        U8 HostProtectedArea  :1;
        U8 Obsolete1  :1;
        U8 WriteBuffer  :1;
        U8 ReadBuffer  :1;
        U8 Nop  :1;
        U8 Obsolete2  :1;//word 85
        U8 DownloadMicrocode  :1;
        U8 DmaQueued  :1;
        U8 Cfa  :1;
        U8 AdvancedPm  :1;
        U8 Msn  :1;
        U8 PowerUpInStandby  :1;
        U8 ManualPowerUp  :1;
        U8 Reserved2  :1;
        U8 SetMax  :1;
        U8 Acoustics  :1;
        U8 BigLba  :1;
        U8 DeviceConfigOverlay  :1;
        U8 FlushCache  :1;
        U8 FlushCacheExt  :1;
        U8 Resrved3  :2;//word 86
        U8 SmartErrorLog  :1;
        U8 SmartSelfTest  :1;
        U8 MediaSerialNumber  :1;
        U8 MediaCardPassThrough  :1;
        U8 StreamingFeature  :1;
        U8 GpLogging  :1;
        U8 WriteFua  :1;
        U8 WriteQueuedFua  :1;
        U8 WWN64Bit  :1;
        U8 URGReadStream  :1;
        U8 URGWriteStream  :1;
        U8 ReservedForTechReport  :2;
        U8 IdleWithUnloadFeature  :1;
        U8 Reserved4  :2;//word 87
    } CommandSetActive;
    U16 UltraDMASupport  :8;
    U16 UltraDMAActive  :8;//word 88
    struct{
        U16 ReportTimeBitsSel :1;//1:bits 14:0 is selected,0:bits 7:0 is selected
        U16 TimeRequired :15;
    }NormalEraseTime;//word 89
    struct{
        U16 ReportTimeBitsSel :1;//1:bits 14:0 is selected,0:bits 7:0 is selected
        U16 TimeRequired :15;
    }EnhancedEraseTime;//word 90
    U16 CurrentApmLevel :8;
    U16 ReservedWord91 : 8;//word 91
    U16 MasterPasswordID;//word 92
    U16 HardwareResetResult;//word 93
    U16 CurrentAcousticValue  :8;
    U16 RecommendedAcousticValue  :8;//word 94
    U16 ReservedWord95[5]; //word 95~99
    U32  Max48BitLBA[2];//word 100~103
    U16 StreamingTransferTime;//word 104
    U16 DataSetManagementMaxBlockNum;//word 105
    struct {
        U16 LogicalSectorsPerPhysicalSector  :4;
        U16 Reserved0  :8;
        U16 LogicalSectorLongerThan256Words  :1;
        U16 MultipleLogicalSectorsPerPhysicalSector  :1;
        U16 Reserved1  :2;
    } PhysicalLogicalSectorSize;//word 106
    U16 InterSeekDelay;//word 107
    U16 WorldWideName[4];//word 108~111
    U16 ReservedForWorldWideName128[4];//word 112~115
    U16 ReservedForTlcTechnicalReport;//word 116
    U16 WordsPerLogicalSector[2];//word 117~118
    struct {
        U16 Obsolete1  :1;
        //U16 ReservedForDrqTechnicalReport  :1;
        U16 WriteReadVerify  :1;
        U16 WriteUeccExtCommand  :1;
        U16 WriteReadLogDMAExtCommand  :1;
        U16 DownloadMicrocodeMode3  :1;
        U16 FreeFallControl  :1;
        U16 SenseDataReporting  :1;
        U16 EPCFeature  :1;
        U16 AccesibleMaxAddressConfig  :1;
        U16 DSNFeature  :1;
        U16 Reserved1  :4;
        U16 ShallBeOne  :1;
        U16 ShallBeZero  :1;
    } CommandSetSupportExt;//word 119
    struct {
        U16 Obsolete1  :1;
        //U16 ReservedForDrqTechnicalReport  :1;
        U16 WriteReadVerify  :1;
        U16 WriteUeccExtCommand  :1;
        U16 WriteReadLogDMAExtCommand  :1;
        U16 DownloadMicrocodeMode3  :1;
        U16 FreeFallControl  :1;
        U16 SenseDataReporting  :1;
        U16 EPCFeature  :1;
        U16 Reserved1  :1;
        U16 DSNFeature  :1;
        U16 Reserved2  :4;
        U16 ShallBeOne  :1;
        U16 ShallBeZero  :1;
    } CommandSetActiveExt;//word 120
    U16 ReservedForExpandedSupportandActive[6];//word 121~126
    U16 MsnSupport  :2;
    U16 ReservedWord127  :14;//word 127
    union {
        struct{
            U8 SecuritySupported  :1;
            U8 SecurityEnabled  :1;
            U8 SecurityLocked  :1;
            U8 SecurityFrozen  :1;
            U8 SecurityCountExpired  :1;
            U8 EnhancedSecurityEraseSupported  :1;
            U8 Reserved0  :2;
            U8 SecurityLevel  :1;//0=High,1=Maximum
            U8 Reserved1  :7;
        };
        U16 AsU16;
    } SecurityStatus;//word 128
    U16 ReservedWord129[31];//word 129~159
    struct {
        U16 MaximumCurrentInMA2  :12;
        U16 CfaPowerMode1Disabled  :1;
        U16 CfaPowerMode1Required  :1;
        U16 Reserved0  :1;
        U16 Word160Supported  :1;
    } CfaPowerModel;//word 160
    U16 ReservedForCfaWord161[7];//word 161~167
    U16 DeviceNorminalFormFactor; //word 168
    struct {
        U16 SupportsTrim  :1;
        U16 Reserved0  :15;
    } DataSetManagementFeature;//word 169
    U16 ReservedForCfaWord170[6];//word 170~175
    U16 CurrentMediaSerialNumber[30];//word 176~205
    U16 ReservedWord206;//word 206
    U16 ReservedWord207[2];//word 207~208
    struct {
        U16 AlignmentOfLogicalWithinPhysical  :14;
        U16 Word209Supported  :1;
        U16 Reserved0  :1;
    } BlockAlignment;//word 209
    U16 WriteReadVerifySectorCountMode3Only[2];//word 210~211
    U16 WriteReadVerifySectorCountMode2Only[2];//word 212~213
    struct {
        U8 NVCachePowerModeEnabled  :1;
        U8 Reserved0  :3;
        U8 NVCacheFeatureSetEnabled  :1;
        U8 Reserved1  :3;
        U8 NVCachePowerModeVersion  :4;
        U8 NVCacheFeatureSetVersion  :4;
    } NVCacheCapabilities;//word 214
    U16 NVCacheSizeLSW;//word 215
    U16 NVCacheSizeMSW;//word 216
    U16 NominalMediaRotationRate;//word 217
    U16 ReservedWord218;//word 218
    struct {
        U8 NVCacheEstimatedTimeToSpinUpInSeconds;
        U8 Reserved;
    } NVCacheOptions;//word 219
    U16 ReservedWord220[2];//word 220~221
    struct{
        U16 ATA8AST :1;
        U16 Sata1_0a :1;
        U16 Sata2Ex :1;
        U16 SataRev2_5 :1;
        U16 SataRev2_6 :1;
        U16 SataRev3_0 :1;
        U16 SataRev3_1 :1;
        U16 SataRev3_2 :1;//byte 0
        U16 Reserved :4;
        U16 TansportType :4;//0-Parallel,1-Serial
    }TransMajorRev;//word 222
    U16 TransMinorRevision;//word 223
    U16 ReservedWord224[6];//word 224~229
    U32 ExtNumOfUserAddrSecLow;
    U32 ExtNumOfUserAddrSecHigh; //word 230~233
    U16 MinDnldMcrCodeSecCnt; //word 234
    U16 MaxDnldMcrCodeSecCnt; //word 235
    U16 ReservedWord236[19]; //word 236~254
    U16 Signature  :8;
    U16 CheckSum  :8;//word 255
} IDENTIFY_DEVICE_DATA, *PIDENTIFY_DEVICE_DATA;

typedef struct _NCQ_CMD_ERR
{
    U8 bsTAG: 5;
    U8 bsRsv0: 1;
    U8 bsUNL: 1; //Unload,1:error is receiving an IDLE IMMEDIATE command with the Unload Feature specified
    U8 bsNQ: 1; //1:bsTAG is not valid
    U8 Rsvd1;
    U8 Status;
    U8 Error;

    U8 LBAByte0;
    U8 LBAByte1;
    U8 LBAByte2;
    U8 Device;

    U8 LBAByte3;
    U8 LBAByte4;
    U8 LBAByte5;
    U8 Rsvd2;

    U8 CountByte0;
    U8 CountByte1;
    U8 Reserved3[242];

    U8 VendorSpecific[255];
    U8 Checksum;
} NCQ_CMD_ERR;

#define SMART_ATTRIBUTE_ENTRY_MAX   30
#define SMART_LOG_DEFAULT_VERSION 1
#define SMART_ATTRIBUTE_VERSION     SMART_LOG_DEFAULT_VERSION
#define SMART_THRESHOLDS_REVISION   SMART_LOG_DEFAULT_VERSION

/* SMART Log Address*/
#define SMART_LOGADDR_LOG_DIRECTORY                   0x00
#define SMART_LOGADDR_SUMMARY_SMART_ERR               0x01
#define SMART_LOGADDR_COMPREHENSIVE_SMART_ERR         0x02
#define SMART_LOGADDR_DEVICE_STATISTICS               0x04
#define SMART_LOGADDR_SMART_SELF_TEST                 0x06
#define SMART_LOGADDR_SELECTIVE_SELF_TEST             0x09
#define SMART_LOGADDR_HOST_SPEC_80H                   0x80

typedef enum SMART_DATA_SECTOR_TAG
{
    SMART_DATA_SECTOR_OFF = 0,
    SMART_THRESHOLDS_SECTOR_OFF,    
    SMART_DIRECTORY_SECTOR_OFF,
    SMART_SUMMARY_ERR_SECTOR_OFF,
    SMART_SELF_TEST_SECTOR_OFF,
    SMART_SELECTIVE_TEST_SECTOR_OFF,
    SMART_COMPREHENSIVE_SMART_ERR_OFF = 6,    
    SMART_HOST_SPEC_80H_OFF = 8,    
    SMART_DATA_SECTOR_MAX
} SMART_DATA_SECTOR;

typedef struct _SMART_ATTRIBUTE_TBL
{
    U8  ucId;
    U8 ucFlaglow;
    U8 ucFlaghi;
    U8  ucValue;

    U8  ucWorst;
    U8  aRowData[6];
    U8  ucRsvd;
}SMART_ATTRIBUTE_TBL, *PSMART_ATTRIBUTE_TBL;

typedef struct _SMART_ATTR_INFO
{
    U16 usVersion;
    SMART_ATTRIBUTE_TBL aAttributeTbl[SMART_ATTRIBUTE_ENTRY_MAX];
    U8  aRsvd[150];
} SMART_ATTR_INFO, *PSMART_ATTR_INFO;

typedef struct _SMART_THRESHOLDS_TBL
{
    U8  ucId;
    U8  ucThresholds;
    U8  aRsvd[10];
}SMART_THRESHOLDS_TBL, *PSMART_THRESHOLDS_TBL;

typedef struct _SMART_THRESHOLDS
{
    U16 usRevision;
    SMART_THRESHOLDS_TBL aThresholdsTbl[SMART_ATTRIBUTE_ENTRY_MAX];
    U8  aRsvd[150];
} SMART_THRESHOLDS, *PSMART_THRESHOLDS;

typedef struct _MEMORIGHT_SMART
{
    U8  ucId;
    U8  ucThresholds;
    U8  ucWorst;
    U8  aName[50];
}MEMORIGHT_SMART, *PMEMORIGHT_SMART;

/* related to WORD128 in identify data, see spec for reference
security status will record in both here and log page, if the status needs
save back for next power up then the it will be updated in log page
concurrently and save log page back
g_usSecurityStatus will be initialize according to the field in log page

in SATA security status, some of the flags shall be saved back when it is
updated, like ENABLED and MASTER CAPABILITY MAX flag, other flags does not
need to save back when it is updated, they will be set according to the saved flag */

#define SATA_SECURITY_SUPPORTED             0x01 
#define SATA_SECURITY_ENABLED               0x02   /*need save back if update*/
#define SATA_SECURITY_LOCKED                0x04 
#define SATA_SECURITY_FROZEN                0x08
#define SATA_SECURITY_COUNTER_EXPIRED       0x10   /*no need to save*/
#define SATA_SECURITY_MASTER_CAPABILITY_MAX 0x0100 /*need save back if update*/

U32 L0_ATASoftResetProc(PCB_MGR pCmdSlot);
U32 L0_ATACmdProcObsoleteCmd(PCB_MGR pSlot);
U32 L0_ATACmdProcIdentifyDevice(PCB_MGR pSlot);
U32 L0_ATACmdProcSetFeatures(PCB_MGR pSlot);
U32 L0_ATACmdProcSetMultipleMode(PCB_MGR pSlot);
U32 L0_ATACmdProcExecDevDiag(PCB_MGR pSlot);
U32 L0_ATACmdProcInitDevParam(PCB_MGR pSlot);
U32 L0_ATACmdProcStandby(PCB_MGR pSlot);
U32 L0_ATACmdCheckPowerMode(PCB_MGR pSlot);
U32 L0_ATACmdProcDatasetMgmt(PCB_MGR pSlot);
U32 L0_ATACmdProcFlushCache(PCB_MGR pSlot);
U32 L0_ATACmdProcVIACmd(PCB_MGR pSlot);
U32 L0_ATACmdProcSmart(PCB_MGR pSlot);
U32 L0_ATACmdProcReadLogExt(PCB_MGR pSlot);
U32 L0_ATACmdProcWriteLogExt(PCB_MGR pSlot);
BOOL L0_ATACmdProcSecurity(PCB_MGR pSlot);
BOOL L0_ATACmdProcHPA(PCB_MGR pSlot);
BOOL L0_ATACmdCheckSecurityStatus(PCB_MGR pSlot);
BOOL L0_ATACmdProcReadBuffer(PCB_MGR pCmdSlot);
BOOL L0_ATACmdProcWriteBuffer(PCB_MGR pCmdSlot);
BOOL L0_ATACmdProcMicroCode(PCB_MGR pCmdSlot);
BOOL L0_ATACmdProcPowerStatusCmd(PCB_MGR pCmdSlot);
BOOL L0_ATAPowerProcIdleToStandby(void *pParam);
void L0_ATAPowerStartStandbyTimer(U32 ulSecondVal);

void L0_ATAInitIdentifyData(void);
void L0_ATAInitSmartData(void);
void L0_ATAInitGPLog(void);
void L0_ATAInitSecurity(void);
void L0_ATAInitHPA(void);
void L0_ATAInitPowerStatus(void);

U32 L0_GPLGetLogPageAddr(U32 ulBuffStart, U32 ulLogAddr, U32 ulPageOffset);

U32 L0_ATACheckNCQCmdCode(U32 ulCmdCode);
void L0_ATASetCmdParam(PCB_MGR pSlot);
U32 L0_ATAGetCmdState(PCB_MGR pSlot);
void L0_ATASetCmdState(PCB_MGR pSlot, U32 ulCmdState);
U32 L0_ATAGenericCmdProc(PCB_MGR pCmdSlot);
U32 L0_ATAMediaAccessProc(PCB_MGR pCmdSlot);
U32 L0_ATAProcessHostCmd(PCB_MGR pCmdSlot);

BOOL L0_ATAIsDatasetMgmtCmdValid(PCFIS pCFis);
BOOL L0_ATAIsMicroCodeCmdValid(PCFIS pCFis);

#endif //__L0_ATA_GENERIC_CMD_LIB_H__

