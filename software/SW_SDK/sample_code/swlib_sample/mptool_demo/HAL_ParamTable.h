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
#define BL_CODE_SIZE (12*1024)
#define FTABLE_SIZE  (256)
#define PTABLE_SIZE  (512)
#define BOOTLOADER_SIZE             (16*1024)
#define MAX_REG_CNT  ((BOOTLOADER_SIZE - FTABLE_SIZE - PTABLE_SIZE)/16)//NOW 216

#define    BOOT_METHOD_NORMAL    0
#define    BOOT_METHOD_LLF       1
#define    BOOT_METHOD_MPT       2
#define    BOOT_METHOD_RSVD      3

typedef union _BOOT_STATIC_FLAG
{
	struct
	{
		U32 bsBootMethodSel : 2;//0:normal; 1:llf;  2:mpt; 3:invalid
		U32 bsWarmBoot : 1;
		U32 bsLocalTest : 1;
		U32 bsRollBackECT : 1;
		U32 bsRebuildGB : 1;
		U32 bsUseDefault : 1;
		U32 bsOptionRomEn : 1;
		U32 bsDebugMode : 1;
		U32 bsUartMpMode : 1;
		U32 bsLLFMethodSel : 3; // 0:normal-llf; 1:read-idb-llf; 2:Format-GBBT-llf; 3:Format-BBT-llf; 4:Format-BBT-with-ReadIDB, other:resvd
		U32 bsExternalSpiFlashEn : 1;
		U32 bsStaticFlagRsv : 18;
	};

	U32 ulStaticFlag;
} BOOT_STATIC_FLAG;


typedef union _BOOT_SELECT_FLAG
{
    struct
    {
        U32 bsEnableMCU0: 1;// TRUE: MCU normal run; FALSE: MCU hang
        U32 bsEnableMCU1: 1;
        U32 bsEnableMCU2: 1;
        U32 bsEnableRsv:29;
    };
    U32 ulEnableMCUFlag;
}BOOT_SELECT_FLAG;

typedef union _HW_INIT_FLAG
{
    struct
    {
        U32 bsGlobalInitDone:1;
        U32 bsDDRInitDone:1;
        U32 bsNFCInitDone:1;
        U32 bsPCIeInitDone:1;
        U32 bsClockGatingInitDone:1;
        U32 bsFlash1STDone:1;
        U32 bsFlash2NDDone:1;
        U32 bsHWInitRsv:25;
    };
    U32 ulHWInitFlag;
} HW_INIT_FLAG;

typedef struct _FW_FEATURE_FLAG
{
    U32 ulFeatureBitMap;
    U32 aFeatureDW[7];
}FW_FEATURE_FLAG;

//define init function type supported in bootloader
typedef enum _INIT_FUNC_TYPE
{
	INIT_GLOBAL,
	INIT_DDR,//DDR init
	INIT_NFC,//NFC init, including set flash to sync mode
	INIT_PCIE,//PCIE configuration
	INIT_CLK_GATING,//clock gating
	INIT_FLASH_1ST,//PM 1st init flash
	INIT_FLASH_2ND,//PM 2nd init flash
	INIT_FUNC_CNT,
	SAVE_FIRWARE = INIT_FUNC_CNT,
	ACTIVE_FIRWARE,
	SAVE_PTABLE,
	RUN_FIRWARE,
	UPGRADE_BL,
	CLEAR_DISK_LOCK,
	FTABLE_FUNC_TOTAL
}INIT_FUNC_TYPE;


//define operation type in register list
typedef enum _REGISTER_OP_TYPE
{
    OPT_REG_SET,//basic reg setting up
    OPT_DELAY,//delay tick cnt
    OPT_NFC_SETFEATURE,//nfc set feature
    OPT_NFC_RESET,//nfc reset
    OPT_NFC_UPDATE_PUMAPPING,//update pu mapping
    OPT_NFC_INTERFACE_INIT,//nfc soft init
    OPT_NONE,//nop
    OPT_FINISH,//finish flag
} REG_OP_TYPE;
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

	/*fw compile date*/
	U32 aFWCompileDate[4]; //4//dword 27 30

	/*fw compile time*/
	U32 aFWCompileTime[4];//4//dword 31 34

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

	/*ptable reserved*/
	U32 aPTableRsvd[(PTABLE_SIZE >> 2) - 105];//the max U32 is 96
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
    
    U32 aFTableRsv[(FTABLE_SIZE>>2)-((FTABLE_FUNC_TOTAL+1)<<2)];
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


#endif/* __HAL_PARAM_TABLE_H__ */

