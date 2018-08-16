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
  File Name     : hscmd_callback.h
  Version       : Initial Draft
  Author        : Betty Wu
  Created       : 2014/9/15
  Last Modified :
  Description   :
  Function List :
  History       :
  1.Date        : 2014/9/15
    Author      : Betty Wu
    Modification: Created file

******************************************************************************/

#ifndef _HSCMD_CALLBACK_H
#define _HSCMD_CALLBACK_H

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
    U16 ReservedWords69[6];//word 69~74
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
        U16 ReservedForDrqTechnicalReport  :1;
        U16 WriteReadVerifySupported  :1;
        U16 Reserved01  :11;
        U16 Reserved1  :2;
    } CommandSetSupportExt;//word 119
    struct {
        U16 ReservedForDrqTechnicalReport  :1;
        U16 WriteReadVerifyEnabled  :1;
        U16 Reserved01  :11;
        U16 Reserved1  :2;
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
    U16 ReservedForCfaWord161[8];//word 161~168
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
    U16 ReservedWord224[31];//word 224~254
    U16 Signature  :8;
    U16 CheckSum  :8;//word 255
} IDENTIFY_DEVICE_DATA, *PIDENTIFY_DEVICE_DATA;

typedef void (*PRFisHandler)(U8 ucCmdTag,U32* pFeedBack);

extern PRFisHandler ParseRFisCallBack;

BOOL ATA_IdentifyCallBack(U8 *pCMDData);
BOOL ATA_SmartReadLogCallBack(U8 *pCMDData);
BOOL ATA_SmartWriteLogCallBack(U8 *pCMDData);
BOOL ATA_SmartReadDataCallBack(U8 *pCMDData);
BOOL ATA_ReadLogExtCallBack(U8 *pCMDData);
BOOL ATA_GetTLInfoCallBack(U8 *pCMDData);
BOOL ATA_GetTLDataCallBack(U8 *pCMDData);
BOOL ATA_GetVarTableCallBack(U8 *pCMDData);


#endif