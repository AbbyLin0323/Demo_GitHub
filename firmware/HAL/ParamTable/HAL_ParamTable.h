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
Filename    : HAL_ParamTable.h
Version     : Ver 1.0
Author      : victorzhang
Date        : 2014.06.18
Description : this file define bootloader/FW interface, inlcuding PTABLE and FTABLE
Others      :
Modify      :
20140618    victorzhang  001 created
20140620    gavinyin     002 add header part of this file
20140909    johnzhang    003 modify definition for optimize/unify code
20140910    Gavin        004 modify it to meet coding style
*******************************************************************************/
#ifndef __HAL_PARAM_TABLE_H__
#define __HAL_PARAM_TABLE_H__
#include "BaseDef.h"
#include "HAL_MemoryMap.h"


/* ASSIC of VT3514C0 */
#define SIGNATURE_DW0    0x35335456
#define SIGNATURE_DW1    0x30433431

/*
BOOTLOADER SIZE:16KBYTE
0~12K BOOTLOADER CODE
12~12.25K FTABLE
12.25~12.625K PTABLE
12.625~16K REGLIST
*/
#define BL_TOTAL_SIZE (BOOTLOADER_PART0_SIZE + BOOTLOADER_PART1_SIZE)

//Henry tmp add
#define BL_TOTAL_SIZE_TMP (0x8000)//32K

#define BL_CODE_SIZE (11*1024)
#define FTABLE_SIZE  (256)
#define PTABLE_SIZE  (512)
#define MAX_REG_CNT  ((BOOTLOADER_PART0_SIZE - FTABLE_SIZE - PTABLE_SIZE - BL_CODE_SIZE)/16)//NOW 216

#define BOOT_METHOD_NORMAL    0
#define BOOT_METHOD_LLF       1
#define BOOT_METHOD_MPT       2
#define BOOT_METHOD_RSVD      3

typedef enum _LLF_METHOD_SEL_
{
    LLF_METHOD_NORMAL = 0,  // load-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_READ_IDB,    // read-idb >>> format-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_FORMAT_GBBT, // format-global-bbt >>> load-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_FORMAT_BBT,  // format-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_REDETECT_IDB,// format-bbt >>> format-disk >>> read-idb >>> save-bbt (for tsb-flash-chip)
    LLF_METHOD_NUM
}LLF_METHOD_SEL;

typedef union _BOOT_STATIC_FLAG
{
    struct
    {
        U32 bsBootMethodSel     : 2;    //0:normal; 1:llf;  2:mpt; 3:invalid
        U32 bsWarmBoot          : 1;
        U32 bsLocalTest         : 1;
        U32 bsRollBackECT       : 1;
        U32 bsRebuildGB         : 1;
        U32 bsUseDefault        : 1;
        U32 bsOptionRomEn       : 1;
        U32 bsDebugMode         : 1;
        U32 bsUartMpMode        : 1;
        U32 bsLLFMethodSel      : 3;    // 0:normal-llf; 1:read-idb-llf; 2:Format-GBBT-llf; 3:Format-BBT-llf; 4:Format-BBT-with-ReadIDB, other:resvd
        U32 bsExternalSpiFlashEn: 1;
        U32 bsRdtTestEnable     : 1;
        U32 bsUartPrintEn       : 1;
        U32 bsInheritEraseCntEn : 1;
        U32 bsPbnBindingTableEn : 1;
        U32 bsRamDiskMode       : 1;    //bit 18,
        U32 bsPrinttoDRAMEn     : 1;    //bit 19, 0: not print to memory; 1: print to memory
        U32 bsTraceLogEn        : 1;    //bit 20, 0: disable tracelog function; 1: enable tracelog function
        U32 bsDataPacketMethod  : 2;    //bit21-22, 00:no data transfer, 01: SATA, 10:UART INT mode, 11: UART polling mode
        U32 bsUart12MEnable     : 1;
        U32 bsStaticFlagRsv     : 8;
    };

    U32 ulStaticFlag;
} BOOT_STATIC_FLAG;

typedef union _BOOT_SELECT_FLAG
{
    struct
    {
        U32 bsEnableMCU0    : 1;    // TRUE: MCU normal run; FALSE: MCU hang
        U32 bsEnableMCU1    : 1;
        U32 bsEnableMCU2    : 1;
        U32 bsEnableRsv     : 29;
    };
    U32 ulEnableMCUFlag;
}BOOT_SELECT_FLAG;

typedef union _HW_INIT_FLAG
{
    struct
    {
        U32 bsGlobalInitDone        : 1;
        U32 bsDDRInitDone           : 1;
        U32 bsNFCInitDone           : 1;
        U32 bsPCIeInitDone          : 1;
        U32 bsClockGatingInitDone   : 1;
        U32 bsFlash1STDone          : 1;
        U32 bsFlash2NDDone          : 1;
        U32 bsHWInitRsv             : 25;
    };
    U32 ulHWInitFlag;
} HW_INIT_FLAG;

typedef struct _FW_FEATURE_FLAG
{
    union
    {
        struct
        {
            U32 bsNVMeL0MSIXEnable  : 2;
            U32 bsNVMeL0LBA4KBEnable: 1;
            U32 bsNVMeL0PMUEnable   : 1;
            U32 bsNVMeL0ForceGen2   : 1;
            U32 res:27;
        }tNVMEL0Feat;

        struct
        {
            U32 bsSATAL0HIPMEnable      : 1;
            U32 bsSATAL0DIPMEnable      : 1;
            U32 bsSATAL0HPAEnable       : 1;
            U32 bsSATAL0SecurityEnable  : 1;
            U32 bsSATAL0SSPEnable       : 1;
            U32 bsSATAL0PMUEnable       : 1;
            U32 res                     : 26;
        }tSATAL0Feat;

        U32 ulFeatureBitMap;
    };
    U32 aFeatureDW[7];
}FW_FEATURE_FLAG;

typedef struct _PRINT_TO_DRAM_INFO
{
    U32 uPrinttoDRAMSize        : 10;
    U32 bsBootLoaderDelayEnable : 1;
    U32 res                     : 9;
    U32 uPrinttoDRAMStartAddr   : 12;
}PRINT_TO_DRAM_INFO;

typedef struct _TRACE_LOG_INFO
{
    U32 uTraceLogDRAMSize       : 8;
    U32 uTraceLogStartAddr_MCU0 : 8;
    U32 uTraceLogStartAddr_MCU1 : 8;
    U32 uTraceLogStartAddr_MCU2 : 8;
}TRACE_LOG_INFO;
/*
//I2C and TSC Temperature sensor setting==================================
//bit[2:0]:For I2C, 0:NOSENSOR 1:G752KC2G; 2:LM73CIMK-0; 3:S-5851AAA-I6T1U; 4:TMP102; 
//bit[3]:For TSC, 0x08:TSC
//bit[4]:Enable TSC and I2C UART Log , 0x10 
//0x19:TSC and I2C with UART log, 0x01:I2C without log, 0x08:TSC without log
define bsTemperatureSensorType 0x08
//I2C Clock 0:100KHz; 1: 400KHz; 2:1MHz;
define bsI2CClock 1
//0x48, 0x49, 0x4a , 0x48: G752KC2G, 0x49:TMP102,0x4A:LM73CIMK
define bsI2CAddr 0x48
//TSC Enable Register 0x1FF80004[15]:0 Disable TSC ; 0x1FF80004[15]:1 Enable TSC
define bsTSCCLKEN  1
//TSC measure time report 0xffe0d800[17:16] 0:1ms 1:10ms 2:100ms 3:1s 
define bsTSCTime 2
//TSC measure time counter to UART value from 1~255 second.   
define bsTSC2UART 10
//TSC smart cmd delta value SATA:18 NVMe:24
define bsDelta 24
//Low threshold 
define bsLoThreshold 32
//High threshold SATA:50 , NVMe:56
define bsHiThreshold 56
*/
typedef struct _TEMPERATURE_SENSOR_I2C
{
    U32 bsTemperatureSensorType : 6;    //0:NOSENSOR 1:G752KC2G; 2:LM73CIMK-0; 3:S-5851AAA-I6T1U;
    U32 bsI2CClock              : 2;    //0:100KHz; 1: 400KHz; 2:1MHz;
    U32 bsI2CAddr               : 8;    //0x48: G752KC2G,0x49:TMP102,0x4A:LM73CIMK
    U32 bsTSCCLKEN              : 4;    //TSC clock enable
    U32 bsTSCTime               : 4;    //TSC measure time
    U32 bsTSC2UART              : 8;    //TSC to UART
}TEMPERATURE_SENSOR_I2C;

typedef struct _TEMPERATURE_SENSOR_TSC
{
    U32 bsDelta:8;           //TSC temperature delta,SATA:18 NVMe:24
    U32 bsLoThreshold :8;    //TSC temperature low threshold, SATA:32 NVMe:32
    U32 bsHiThreshold:8;     //TSC temperature high threshold, SATA:50 NVMe:56
    U32 bsReserved:8;        //Reserved byte
 }TEMPERATURE_SENSOR_TSC;

//define init function type supported in bootloader
typedef enum _INIT_FUNC_TYPE
{
    INIT_GLOBAL,
    INIT_DDR,       //DDR init
    INIT_NFC,       //NFC init, including set flash to sync mode
    INIT_PCIE,      //PCIE configuration
    INIT_CLK_GATING,//clock gating
    INIT_FLASH_1ST, //PM 1st init flash
    INIT_FLASH_2ND, //PM 2nd init flash
    INIT_FUNC_CNT,
    SAVE_FIRWARE = INIT_FUNC_CNT,
    ACTIVE_FIRWARE,
    SAVE_PTABLE,
    RUN_FIRWARE,
    UPGRADE_BL,
    CLEAR_DISK_LOCK,
    GET_HEX,
    GET_STRING,
    FTABLE_FUNC_TOTAL
}INIT_FUNC_TYPE;

//define operation type in register list
typedef enum _REGISTER_OP_TYPE
{
    OPT_REG_SET,                //basic reg setting up
    OPT_DELAY,                  //delay tick cnt
    OPT_NFC_SETFEATURE,         //nfc set feature
    OPT_NFC_RESET,              //nfc reset
    OPT_NFC_UPDATE_PUMAPPING,   //update pu mapping
    OPT_NFC_INTERFACE_INIT,     //nfc soft init
    OPT_NONE,                   //nop
    OPT_FINISH,                 //finish flag
} REG_OP_TYPE;


/* firmware version information */
#define FW_VERSION_INFO_MCU_TYPE_NONE       0
#define FW_VERSION_INFO_MCU_TYPE_B0         1
#define FW_VERSION_INFO_MCU_TYPE_C0         2

#define FW_VERSION_INFO_HOST_TYPE_NONE      0
#define FW_VERSION_INFO_HOST_TYPE_SATA      1
#define FW_VERSION_INFO_HOST_TYPE_AHCI      2
#define FW_VERSION_INFO_HOST_TYPE_NVME      3

#define FW_VERSION_INFO_FLASH_TYPE_NONE     0
#define FW_VERSION_INFO_FLASH_TYPE_L85      1
#define FW_VERSION_INFO_FLASH_TYPE_L95      2
#define FW_VERSION_INFO_FLASH_TYPE_A19      3
#define FW_VERSION_INFO_FLASH_TYPE_A19_4PLN 4

#define FW_VERSION_INFO_CONFIGURATION_NONE  0
#define FW_VERSION_INFO_CONFIGURATION_VPU   1

typedef union _FW_VERSION_INFO
{
    struct
    {
        /* BYTE 0: MCU Core Types */
        U8 ucMCUType;

        /* BYTE 1: Host Types */
        U8 ucHostType;

        /* BYTE 2: Flash Types */
        U8 ucFlashType;

        /* BYTE 3: Other Configurations */
        U8 ucOtherConfig;

        /* BYTE 4-7: Bootloader Parameter Types (Only for BL, FW reserved) */
        U32 ulBLParamTypes;

        /* BYTE 8-11: FW release version */
        U32 ulFWRleaseVerion;

        /* BYTE 12-15: GIT release version */
        U32 ulGITVersion;

        /* BYTE 16-19: Compile Date */
        U32 ulDateInfo;  /* 20150528 */

        /* BYTE 20-23: Compile Time */
        U32 ulTimeInfo;  /* 00163410 */
    };

    U32 ulFWVersion[6];
} FW_VERSION_INFO;


typedef union _FW_RUNTIME_INFO
{
    struct
    {
        /* DWORD 0-5: FW version info */
        FW_VERSION_INFO FWVersionInfo;

        /* DWORD 6-11: Bootloader version info */
        FW_VERSION_INFO BLVersionInfo;

        /* DWORD 12-17: Flash detail info */
        U32 ucPlnNum:8;
        U32 ucLunNum:8;
        U32 usBlockPerCE:16;
        U32 usPagePerBlock:16;
        U32 usPhyPageSize:16;
        U32 ulFlashId[2];
        U32 ulPhyCeMap[2];

        /* DWORD 18-22: Disk config info */
        U32 ucCeCount:8;
        U32 ucMcuCount:8;
        U32 usPad:16;
        U32 ulDRAMSize;
        U32 ulMcuConfigBytes[3];

        /* DWORD 23-27: Reserved */
        U32 ulRsvd[5];
    };

    U32 ulFWRunTimeInfo[28];
} FW_RUNTIME_INFO;

typedef union _FLASH_INFO
{
    struct
    {
      U32 ulLunNum;
      U8 aFlashName[28];
    };
    U32 aFlahsInfo[8];
}FLASH_INFO;

/*  FT Parameters: 10 DW pending  */
typedef struct _FT_PARAM_
{
    //DW 0
    union{
        U32 FT_PARAM_DW0;
        struct
        {
            U32 ucPu:8;
            U32 ucTestPuNum:8;
            U32 bsPrintPage:1;
            U32 bsCheckCTT:1;
            U32 bsEnablePartialWLTest:1;
            U32 bsEnableCheckData:1;
            U32 bsUseDramData:1;
            U32 bsRetentionRead:1;
            U32 bsEnableShiftRead:1;
            U32 bsLatencytest:1;
            U32 ucDataPatternSelect:8;
        };
    };

    //DW 1
    union{
        U32 FT_PARAM_DW1;
        struct
        {
            U16 usBlock;
            U16 usTestBlockNum;
        };
    };
    
    //DW 2
    U32 ulPECount; //total test PE count

    //DW 3
    U32 ulReadPoint; 
    
    //DW 4
    U32 ulCurrentPECount;
    
    //DW 5
    U32 ulDwellTime_PE;
    
    //DW 6
    U32 ulDwellTime_Read_1;
#if 0   
    //DW 7
    U32 ulDwellTime_Read_2;
    
    //DW 8
    U32 ulRetentionReadLoop;
    
    //DW 9
    union{
        U32 FT_PARAM_DW9;
        struct
        {
            U32 bsPrintCTTInfo:1;
            U32 bsEnableFwScramble:1;
            U32 bsEnableFwGenerateLFSR:1;
            U32 bsEnableFwAsym:1;
            U32 rsvd:12;
            U32 ucMaxRetryTime:8;
            U32 ucCurrentRetryTime:8;
        };
    };

    //DW 10
    union{
        U32 FT_PARAM_DW10;
        struct
        {
            U32 ulCurrentPage:10;
            U32 bsEnableFlashManage:1;
            U32 bsNewFlash:1;
            U32 DW10Rsv:20;
        };
    };
#endif
}FT_PARAM;


//param table
typedef struct _PTABLE
{
    /*ptable version*/
    FW_VERSION_INFO tBLVersion; //dword 0~5

    /*
    static flag for algorithm
    */
    BOOT_STATIC_FLAG sBootStaticFlag;//dword 6

    /*
    Control MCU's behavior after boot: hang, or normal run
    */
    BOOT_SELECT_FLAG sBootSelFlag;//dword 7

    /*
    HW init done flag: TRUE means Bootloader finished init sequence
    */
    HW_INIT_FLAG sHwInitFlag;//dword 8

    /*
    Including fw major minor version and svn sub version
    */
    U32 aFWVersion[2];////dword 9 10

    /*
    Identify data fw disk name 40bytes
    */
    U32 aFWDiskName[10];//11 20

    /*
    Identify data fw sn 20bytes
    */
    U32 aFWSerialNum[5];//21 25

    /*fw saved page cnt:fwsize/pgsize*/
    U32 ulFWSavePageCnt;//26

    /* Flash Name & Lun count */
    FLASH_INFO tFlashInfo;//dword 27~34

    /*
    ulSubSysNum 1 or 2 means,this fw support one core subsys or two core subsys
    */
    U32 ulSubSysNum;//dword 35

    /*CE bitmap for sub system0/1, "bit = 1" means enable CE for sub system*/
    U32 ulSubSysCEMap[2];//dword 36 37

    /*sub system ce number*/
    U32 ulSubSysCeNum;//dword 38

    /*sub system pln number*/
    U32 ulSubSysPlnNum;//dword 39

    /*sub system block number*/
    U32 ulSubSysBlkNum;//dword 40

    /*sub system reserved block number*/
    U32 ulSubSysRsvdBlkNum;//dword 41

    /*sub system page number*/
    U32 ulSubSysFWPageNum;//dword 42

    /* flash physic page size*/
    U32 ulFlashPageSize;//dword 43

    /*sub system max lba cnt*/
    U32 ulSysMaxLBACnt;//dword 44

    /*disk world wide name*/
    U32 ulWorldWideName[2];//dword 45 46

    /* flash id */
    U32 aFlashChipId[2];//dword 47 48

    /* FW L0 Feature */
    FW_FEATURE_FLAG tSATAL0Feature; //dword 49~56

    FW_FEATURE_FLAG tAHCIL0Feature; //dword 57~64

    FW_FEATURE_FLAG tNVMeL0Feature; //dword 65~72

    /*FW L1 Feature*/
    FW_FEATURE_FLAG tL1Feature; //dword 73~80

    /*FW L2 Feature*/
    FW_FEATURE_FLAG tL2Feature; //dword 81~88

    /*FW L3 Feature*/
    FW_FEATURE_FLAG tL3Feature; //dword 89~96

    /*FW HAL Feature*/
    FW_FEATURE_FLAG tHALFeature; //dword 97~104
    U32 tRdtTable[12]; //dword 105~116

    /*Temperature sensor relative*/
    TEMPERATURE_SENSOR_I2C tTemperatureSensorI2C; //dword 117

    /*Flash P/E Cycle Value*/
    U32 ulFlashPECycleVal;  //dword 118

    PRINT_TO_DRAM_INFO tPrintToDRAMFeature; // dword 119

    /*For Smart cmd TSC new profile value*/
    TEMPERATURE_SENSOR_TSC tTemperatureSensorTSC; //dword 120

    TRACE_LOG_INFO tTraceLogDRAMFeature; // dword121
    /*ptable reserved*/
    //U32 aPTableRsvd[(PTABLE_SIZE>>2)-119];//the total U32 128
    //U32 aPTableRsvd[(PTABLE_SIZE>>2)-120];//the total U32 128
#ifdef FLASH_TEST
    FT_PARAM tFTParam;
#else
    U32 aPTableRsvd[(PTABLE_SIZE>>2)-122];//the total U32 128
#endif
}PTABLE;

/* function entry and param for init sequence  */
typedef struct _INIT_FUNC_ENTRY
{
    /*function address generated in bootloader compile stage*/
    U32 ulEntryAddr;

    /*ulRegPos : offset in reglist(dword); ulRegCnt: function reg count*/
    U32 ulRegPos;
    U32 ulRegCnt;

    U32 ulRsvd;
}INIT_FUNC_ENTRY;

//function table:
typedef struct _FTABLE
{
    /*ftable version*/
    U32 Version;

    /*jtag mode entry in jtag download fw.elf mode, ulEntryJtag=boot loader jump to
    fw point,now with bin download,it is not used*/
    U32 ulEntryJtag;

    /* reserved for make aInitFuncEntry[] start at 4DW */
    U32 aRsvd[2];

    /*function entry list*/
    INIT_FUNC_ENTRY aInitFuncEntry[FTABLE_FUNC_TOTAL]; //INIT_FUNC_CNT max is 15

    /*  Henry mark this because WinSim compiler will report "illegal zero-sized array"  */
    //U32 aFTableRsv[(FTABLE_SIZE>>2)-((FTABLE_FUNC_TOTAL+1)<<2)];
}FTABLE;

//format of every register operation entry
typedef struct _REG_ENTRY
{
    U32 ulOpcode;
    U32 ulAddr;
    U32 ulAndMsk;
    U32 ulOrMsk;
}REG_ENTRY;

typedef struct _REG_LIST
{
    REG_ENTRY aReg[MAX_REG_CNT];
}REG_LIST;

typedef struct _BOOTLOADER_FILE
{
    U32 ulBLSignatureDW0;
    U32 ulBLSignatureDW1;
    U8 ucBootCode[BL_CODE_SIZE - 8];
    FTABLE tSysFunctionTable;
    PTABLE tSysParameterTable;
    REG_LIST tSysRegList;
}BOOTLOADER_FILE;

extern GLOBAL U32 g_ulPuNum;    // Added for suit Dynamic PU_NUM func

typedef void (*FunPointer)(void);
typedef BOOL (*pSaveFW)(U8 ucSlot, U32 ulDramAddr);
typedef BOOL (*pActiveFW)(U8 ucSlotID);
typedef void (*pSavePTable)(void);
typedef void (*pRunFW)(void);
typedef void (*pUpgradeBL)(U32 ulSrcBootloaderAddr, U32 ulImgBuf,U32 ulRedBuf);
typedef void (*pClearDiskLock)(U32 ulBufferForGB);


PTABLE * HAL_GetPTableAddr(void);
BOOL HAL_IsBootLoaderFileExist(void);
void HAL_InitGlobalPUNum(void);
void HAL_SetPTableDoneBit(U8 ucFuncType);
void HAL_ClearPTableDoneBit(U8 ucFuncType);
BOOL HAL_IsPTableDone(U8 ucFuncType);
BOOL HAL_IsPTableAllDone(void);
BOOL HAL_IsFTableValid(U8 ucFuncType);
BOOL HAL_IsFTableAllValid(void);
void HAL_RegisterFTableFunc(U8 ucFuncType, U32 ulFuncAddr);
void HAL_InvokeFTableFunc(U8 ucFuncType);
U32 HAL_GetFTableFuncAddr(U8 ucFuncType);
U32 HAL_GetSubSystemCEMap(U32 ulMcuId);
void HAL_SetDebugMode(void);
BOOL HAL_ISDebugMode(void);
BOOL HAL_IsMCUEnable(U32 ulMcuId);
void HAL_PrepareFlagsForNormalBoot(void);
U32 HAL_GetFlashChipId(U8 ucIndex);
U8 HAL_GetBootMethod(void);
BOOL HAL_GetInheritEraseCntEnFlag(void);
BOOL HAL_IsRamDiskMode(void);
BOOL HAL_GetPrinttoDRAMEnFlag(void);
BOOL HAL_GetWarmBootFlag(void);
U32 HAL_GetPrintDRAMStartAddr(void);
U32 HAL_GetPrintDRAMSize(void);
BOOL HAL_IsBootLoaderNeedDelay(void);
U8 HAL_GetDataPacketMethod(void);
TEMPERATURE_SENSOR_TSC * HAL_GetTSCSmartValue(void);
TEMPERATURE_SENSOR_I2C * HAL_GetTemperatureSensorI2C(void);
BOOL HAL_IsBaudRate12MEnable();
#endif/* __HAL_PARAM_TABLE_H__ */

